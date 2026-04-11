/********************************************************************
* file: audio_play_demo.c              date: 日 2026-04-12 10:00:00 *
*                                                                   *
* Description: Send talkback audio frames to /run/audio_play/control*
*                                                                   *
* Maintainer:  (yxl)         <852041654@qq.com>                     *
*                                                                   *
* This file is free software;                                       *
*   you are free to modify and/or redistribute it                   *
*   under the terms of the GNU General Public Licence (GPL).        *
*                                                                   *
* No warranty, no liability, use this at your own risk!             *
********************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef MSG_EOR
#define MSG_EOR 0
#endif

#define DEFAULT_SOCKET_PATH "/run/audio_play/control"
#define FRAME_HEADER_SIZE 4
#define FRAME_PAYLOAD_SIZE 640
#define FRAME_SIZE (FRAME_HEADER_SIZE + FRAME_PAYLOAD_SIZE)
#define DEFAULT_FRAME_US 40000
#define DEFAULT_TONE_HZ 1000.0
#define DEFAULT_SAMPLE_RATE 16000.0
#define DEFAULT_SECONDS 3
#define SILENCE_ALAW 0xD5

static volatile sig_atomic_t g_stop = 0;

enum play_mode {
	MODE_SILENCE = 0,
	MODE_TONE,
	MODE_FILE
};

struct options {
	const char *socket_path;
	enum play_mode mode;
	const char *file_path;
	int seconds;
	int frame_us;
	double tone_hz;
};

static void signal_handler(int signo)
{
	(void)signo;
	g_stop = 1;
}

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage:\n"
		"  %s silence [seconds] [socket_path]\n"
		"  %s tone [seconds] [socket_path] [freq_hz]\n"
		"  %s file <alaw_file> [socket_path] [frame_us]\n"
		"\n"
		"Protocol:\n"
		"  4-byte zero header + 640-byte A-law payload per packet\n"
		"\n"
		"Examples:\n"
		"  %s silence 3\n"
		"  %s tone 5 /run/audio_play/control 1000\n"
		"  %s file test.alaw /run/audio_play/control 40000\n",
		prog, prog, prog, prog, prog, prog);
}

static int parse_int(const char *s, int *value)
{
	char *end = NULL;
	long v;

	errno = 0;
	v = strtol(s, &end, 10);
	if (errno != 0 || end == s || *end != '\0') {
		return -1;
	}
	if (v < 0 || v > 2147483647L) {
		return -1;
	}
	*value = (int)v;
	return 0;
}

static int parse_double(const char *s, double *value)
{
	char *end = NULL;
	double v;

	errno = 0;
	v = strtod(s, &end);
	if (errno != 0 || end == s || *end != '\0') {
		return -1;
	}
	*value = v;
	return 0;
}

static int parse_args(int argc, char **argv, struct options *opt)
{
	memset(opt, 0, sizeof(*opt));
	opt->socket_path = DEFAULT_SOCKET_PATH;
	opt->mode = MODE_TONE;
	opt->seconds = DEFAULT_SECONDS;
	opt->frame_us = DEFAULT_FRAME_US;
	opt->tone_hz = DEFAULT_TONE_HZ;

	if (argc < 2) {
		usage(argv[0]);
		return -1;
	}

	if (strcmp(argv[1], "silence") == 0) {
		opt->mode = MODE_SILENCE;
		if (argc >= 3 && parse_int(argv[2], &opt->seconds) < 0) {
			fprintf(stderr, "invalid seconds: %s\n", argv[2]);
			return -1;
		}
		if (argc >= 4) {
			opt->socket_path = argv[3];
		}
		return 0;
	}

	if (strcmp(argv[1], "tone") == 0) {
		opt->mode = MODE_TONE;
		if (argc >= 3 && parse_int(argv[2], &opt->seconds) < 0) {
			fprintf(stderr, "invalid seconds: %s\n", argv[2]);
			return -1;
		}
		if (argc >= 4) {
			opt->socket_path = argv[3];
		}
		if (argc >= 5 && parse_double(argv[4], &opt->tone_hz) < 0) {
			fprintf(stderr, "invalid tone frequency: %s\n", argv[4]);
			return -1;
		}
		return 0;
	}

	if (strcmp(argv[1], "file") == 0) {
		opt->mode = MODE_FILE;
		if (argc < 3) {
			usage(argv[0]);
			return -1;
		}
		opt->file_path = argv[2];
		if (argc >= 4) {
			opt->socket_path = argv[3];
		}
		if (argc >= 5 && parse_int(argv[4], &opt->frame_us) < 0) {
			fprintf(stderr, "invalid frame_us: %s\n", argv[4]);
			return -1;
		}
		return 0;
	}

	usage(argv[0]);
	return -1;
}

static int unix_sock_connect(const char *path)
{
	struct sockaddr_un addr;
	int fd;

	fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	if (strlen(path) >= sizeof(addr.sun_path)) {
		fprintf(stderr, "socket path too long: %s\n", path);
		close(fd);
		return -1;
	}
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		close(fd);
		return -1;
	}

	return fd;
}

static unsigned char linear_to_alaw(int pcm_val)
{
	static const int seg_end[8] = {
		0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF
	};
	int mask;
	int seg;
	unsigned char aval;

	if (pcm_val >= 0) {
		mask = 0xD5;
	} else {
		mask = 0x55;
		pcm_val = -pcm_val - 1;
	}

	if (pcm_val > 0x0FFF) {
		pcm_val = 0x0FFF;
	}

	for (seg = 0; seg < 8; seg++) {
		if (pcm_val <= seg_end[seg]) {
			break;
		}
	}

	if (seg >= 8) {
		return (unsigned char)(0x7F ^ mask);
	}

	aval = (unsigned char)(seg << 4);
	if (seg < 2) {
		aval |= (unsigned char)((pcm_val >> 4) & 0x0F);
	} else {
		aval |= (unsigned char)((pcm_val >> (seg + 3)) & 0x0F);
	}

	return aval ^ (unsigned char)mask;
}

static void build_silence_payload(uint8_t *payload)
{
	memset(payload, SILENCE_ALAW, FRAME_PAYLOAD_SIZE);
}

static void build_tone_payload(uint8_t *payload, double *phase, double freq_hz)
{
	int i;
	double local_phase = *phase;
	double step = (2.0 * M_PI * freq_hz) / DEFAULT_SAMPLE_RATE;

	for (i = 0; i < FRAME_PAYLOAD_SIZE; i++) {
		double sample = sin(local_phase) * 12000.0;
		int pcm = (int)sample;
		payload[i] = linear_to_alaw(pcm);
		local_phase += step;
		if (local_phase >= 2.0 * M_PI) {
			local_phase -= 2.0 * M_PI;
		}
	}

	*phase = local_phase;
}

static int build_file_payload(int fd, uint8_t *payload)
{
	ssize_t got = 0;

	while (got < FRAME_PAYLOAD_SIZE) {
		ssize_t ret = read(fd, payload + got, FRAME_PAYLOAD_SIZE - got);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			}
			perror("read");
			return -1;
		}
		if (ret == 0) {
			if (got == 0) {
				return 0;
			}
			memset(payload + got, SILENCE_ALAW, FRAME_PAYLOAD_SIZE - got);
			return 1;
		}
		got += ret;
	}

	return 1;
}

static int send_frame(int sock_fd, const uint8_t *payload)
{
	uint8_t header[FRAME_HEADER_SIZE] = {0};
	struct iovec iov[2];
	struct msghdr msg;
	ssize_t ret;

	memset(&msg, 0, sizeof(msg));
	iov[0].iov_base = header;
	iov[0].iov_len = sizeof(header);
	iov[1].iov_base = (void *)payload;
	iov[1].iov_len = FRAME_PAYLOAD_SIZE;
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;

	ret = sendmsg(sock_fd, &msg, MSG_EOR | MSG_NOSIGNAL);
	if (ret < 0) {
		perror("sendmsg");
		return -1;
	}
	if (ret != FRAME_SIZE) {
		fprintf(stderr, "short send: %zd\n", ret);
		return -1;
	}
	return 0;
}

static void sleep_frame(int frame_us)
{
	struct timespec req;

	req.tv_sec = frame_us / 1000000;
	req.tv_nsec = (long)(frame_us % 1000000) * 1000L;
	while (nanosleep(&req, &req) < 0 && errno == EINTR && !g_stop) {
	}
}

int main(int argc, char **argv)
{
	struct options opt;
	uint8_t payload[FRAME_PAYLOAD_SIZE];
	double phase = 0.0;
	int sock_fd = -1;
	int file_fd = -1;
	int frames_left = -1;
	int ret = 1;

	if (parse_args(argc, argv, &opt) < 0) {
		return 1;
	}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	sock_fd = unix_sock_connect(opt.socket_path);
	if (sock_fd < 0) {
		return 1;
	}

	if (opt.mode == MODE_FILE) {
		file_fd = open(opt.file_path, O_RDONLY | O_CLOEXEC);
		if (file_fd < 0) {
			perror("open input file");
			goto out;
		}
	}

	if (opt.mode != MODE_FILE) {
		frames_left = (opt.seconds * 1000000) / opt.frame_us;
		if (frames_left <= 0) {
			frames_left = 1;
		}
	}

	fprintf(stderr,
		"audio_play_demo: mode=%s socket=%s frame_us=%d\n",
		opt.mode == MODE_SILENCE ? "silence" :
		(opt.mode == MODE_TONE ? "tone" : "file"),
		opt.socket_path, opt.frame_us);

	while (!g_stop) {
		if (opt.mode == MODE_SILENCE) {
			build_silence_payload(payload);
		} else if (opt.mode == MODE_TONE) {
			build_tone_payload(payload, &phase, opt.tone_hz);
		} else {
			int file_ret = build_file_payload(file_fd, payload);
			if (file_ret < 0) {
				goto out;
			}
			if (file_ret == 0) {
				break;
			}
		}

		if (send_frame(sock_fd, payload) < 0) {
			goto out;
		}

		if (frames_left > 0) {
			frames_left--;
			if (frames_left == 0) {
				break;
			}
		}

		sleep_frame(opt.frame_us);
	}

	ret = 0;

out:
	if (file_fd >= 0) {
		close(file_fd);
	}
	if (sock_fd >= 0) {
		close(sock_fd);
	}
	return ret;
}
