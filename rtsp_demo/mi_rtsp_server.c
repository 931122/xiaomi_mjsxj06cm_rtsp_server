/********************************************************************
* file: mi_rtsp_server.c               date: 六 2024-08-03 11:00:16 *
*                                                                   *
* Description: Xiaomi MJSXJ06CM RTSP server                         *
*                                                                   *
* Maintainer:  (yxl)         <852041654@qq.com>                     *
*                                                                   *
* This file is free software;                                       *
*   you are free to modify and/or redistribute it                   *
*   under the terms of the GNU General Public Licence (GPL).        *
*                                                                   *
* Last modified:                                                    *
*                                                                   *
* No warranty, no liability, use this at your own risk!             *
********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <unistd.h>

#include "rtsp_demo.h"
#include "mi_unix.h"
#include "mi_audio_codec.h"
#include "mi_audio_play.h"

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#define MAX_BUFFER_SIZE        128
#define SHM_HEADER_SIZE        96
#define EPOLL_WAIT_TIMEOUT_MS  5
#define RECONNECT_INTERVAL_US  1000000ULL
#define MAX_RECV_BUDGET        32
#define MAX_PTS_JUMP_US        5000000ULL
#define AUDIO_PLAY_PATH        "/run/audio_play/control"

struct mi_stream_info {
	const char *unix_path;
	const char *stream_path;
	size_t pts_offset;
	size_t len_offset;
	bool is_audio;
	bool enabled;
	int fd;
	uint64_t last_retry_us;
	struct epoll_event ev;
	rtsp_session_handle session;
};

static struct mi_stream_info mi_info[3] = {
	{ "/run/video_mainstream/control", "/main", 72, 80, false, true, -1, 0, {0}, NULL },
	{ "/run/video_substream/control",  "/sub",  72, 80, false, true, -1, 0, {0}, NULL },
	{ "/var/run/audio_in/control",     NULL,    80, 88, true,  true, -1, 0, {0}, NULL },
};

struct mi_clock_sync {
	bool initialized;
	uint64_t base_dev_pts;
	uint64_t base_rel_ts;
	uint64_t last_dev_pts;
};

static volatile sig_atomic_t flag_run = 1;

static void sig_proc(int signo)
{
	(void)signo;
	flag_run = 0;
}

static uint64_t mi_normalize_pts(struct mi_clock_sync *sync, uint64_t dev_pts_us)
{
	uint64_t now_rel = rtsp_get_reltime();

	if (dev_pts_us == 0) {
		return now_rel;
	}

	if (!sync->initialized) {
		sync->initialized = true;
		sync->base_dev_pts = dev_pts_us;
		sync->base_rel_ts = now_rel;
		sync->last_dev_pts = dev_pts_us;
		return now_rel;
	}

	if (dev_pts_us < sync->base_dev_pts ||
	    (dev_pts_us > sync->last_dev_pts && (dev_pts_us - sync->last_dev_pts) > MAX_PTS_JUMP_US) ||
	    (dev_pts_us < sync->last_dev_pts && (sync->last_dev_pts - dev_pts_us) > MAX_PTS_JUMP_US)) {
		sync->base_dev_pts = dev_pts_us;
		sync->base_rel_ts = now_rel;
		sync->last_dev_pts = dev_pts_us;
		return now_rel;
	}

	if (dev_pts_us > sync->last_dev_pts) {
		sync->last_dev_pts = dev_pts_us;
	}

	return sync->base_rel_ts + (dev_pts_us - sync->base_dev_pts);
}

static void disconnect_stream(int epoll_fd, struct mi_stream_info *stream)
{
	if (stream->fd >= 0) {
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, stream->fd, NULL);
		close(stream->fd);
		stream->fd = -1;
	}
}

static int connect_stream(int epoll_fd, struct mi_stream_info *stream, uint64_t now_us)
{
	struct msghdr msg;
	struct iovec iov[2];
	char control[CMSG_SPACE(sizeof(int))];
	char iovPart1[4];
	char iovPart2[MAX_BUFFER_SIZE];

	stream->last_retry_us = now_us;
	stream->fd = mi_unix_sock_connect_seqpacket(stream->unix_path, 1);
	if (stream->fd < 0) {
		return -1;
	}

	memset(&msg, 0, sizeof(msg));
	memset(control, 0, sizeof(control));
	iov[0].iov_base = iovPart1;
	iov[0].iov_len = sizeof(iovPart1);
	iov[1].iov_base = iovPart2;
	iov[1].iov_len = sizeof(iovPart2);
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);

	/* Consume initial handshake/metadata packet if already queued. */
	if (recvmsg(stream->fd, &msg, MSG_DONTWAIT) > 0 &&
	    !(msg.msg_flags & MSG_CTRUNC) &&
	    msg.msg_controllen >= CMSG_LEN(sizeof(int))) {
		struct cmsghdr *cm = CMSG_FIRSTHDR(&msg);
		if (cm && cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SCM_RIGHTS &&
		    cm->cmsg_len >= CMSG_LEN(sizeof(int))) {
			int init_fd = *(int *)CMSG_DATA(cm);
			if (init_fd >= 0) {
				close(init_fd);
			}
		}
	}

	stream->ev.events = EPOLLIN;
	stream->ev.data.ptr = stream;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, stream->fd, &stream->ev) < 0) {
		perror("epoll_ctl add");
		close(stream->fd);
		stream->fd = -1;
		return -1;
	}

	printf("connected %s\n", stream->unix_path);
	return 0;
}

static void dispatch_audio_frame(struct mi_alaw_downsampler *ds,
				 uint32_t audio_rate,
				 const uint8_t *payload,
				 int payload_len,
				 uint64_t ts)
{
	uint8_t audio_send_buf[1024];
	const uint8_t *out_data = payload;
	int out_len = payload_len;
	int ch;

	if (audio_rate == 8000) {
		out_len = mi_alaw_downsampler_process(ds, payload, payload_len,
						      audio_send_buf, sizeof(audio_send_buf));
		if (out_len <= 0) {
			return;
		}
		out_data = audio_send_buf;
	}

	for (ch = 0; ch < 2; ch++) {
		if (mi_info[ch].session && rtsp_session_client_count(mi_info[ch].session) > 0) {
			rtsp_tx_audio(mi_info[ch].session, out_data, out_len, ts);
		}
	}
}

static void drain_stream_events(int epoll_fd,
				struct mi_stream_info *stream,
				struct mi_clock_sync *clock_sync,
				struct mi_alaw_downsampler *audio_ds,
				uint32_t audio_rate)
{
	struct msghdr msg;
	struct iovec iov[2];
	char control[CMSG_SPACE(sizeof(int))];
	char iovPart1[4];
	char iovPart2[MAX_BUFFER_SIZE];
	int budget = MAX_RECV_BUDGET;

	iov[0].iov_base = iovPart1;
	iov[0].iov_len = sizeof(iovPart1);
	iov[1].iov_base = iovPart2;
	iov[1].iov_len = sizeof(iovPart2);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	msg.msg_control = control;

	while (budget-- > 0) {
		int map_len = 0;
		int map_fd = -1;
		uint8_t *map_addr;
		uint32_t msg_len = 0;
		uint64_t dev_pts_us = 0;
		uint64_t ts;
		ssize_t len;

		memset(control, 0, sizeof(control));
		msg.msg_controllen = sizeof(control);
		len = recvmsg(stream->fd, &msg, MSG_DONTWAIT);
		if (len < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			if (errno == EINTR) {
				continue;
			}
			printf("recv %s err: %s\n", stream->unix_path, strerror(errno));
			disconnect_stream(epoll_fd, stream);
			break;
		}
		if (len == 0) {
			printf("stream %s closed by peer\n", stream->unix_path);
			disconnect_stream(epoll_fd, stream);
			break;
		}

		if (!(msg.msg_flags & MSG_CTRUNC) && msg.msg_controllen >= CMSG_LEN(sizeof(int))) {
			struct cmsghdr *cm = CMSG_FIRSTHDR(&msg);
			if (cm && cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SCM_RIGHTS &&
			    cm->cmsg_len >= CMSG_LEN(sizeof(int))) {
				map_fd = *(int *)CMSG_DATA(cm);
			}
		}

		if (len < 12) {
			if (map_fd >= 0) {
				close(map_fd);
			}
			continue;
		}

		memcpy(&map_len, &iovPart2[4], sizeof(map_len));
		if (unlikely(map_fd < 0)) {
			continue;
		}
		if (map_len <= SHM_HEADER_SIZE) {
			close(map_fd);
			continue;
		}

		map_addr = mmap(NULL, (size_t)map_len, PROT_READ, MAP_PRIVATE, map_fd, 0);
		if (unlikely(map_addr == MAP_FAILED)) {
			close(map_fd);
			printf("mmap %s failed: %s\n", stream->unix_path, strerror(errno));
			continue;
		}

		memcpy(&msg_len, map_addr + stream->len_offset, sizeof(msg_len));
		if (msg_len == 0 || msg_len > (uint32_t)(map_len - SHM_HEADER_SIZE)) {
			munmap(map_addr, (size_t)map_len);
			close(map_fd);
			continue;
		}

		memcpy(&dev_pts_us, map_addr + stream->pts_offset, sizeof(dev_pts_us));
		ts = mi_normalize_pts(clock_sync, dev_pts_us);

		if (stream->is_audio) {
			dispatch_audio_frame(audio_ds, audio_rate,
					     map_addr + SHM_HEADER_SIZE, (int)msg_len, ts);
		} else if (stream->session) {
			rtsp_tx_video(stream->session,
				      map_addr + SHM_HEADER_SIZE, (int)msg_len, ts);
		}

		munmap(map_addr, (size_t)map_len);
		close(map_fd);
	}
}

static void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s [port] [--audio-rate 8000|16000]\n", prog);
}

int main(int argc, char *argv[])
{
	int epoll_fd;
	int ret, ch = 0;
	int port = 0;
	uint32_t audio_rate = 8000;
	rtsp_demo_handle demo;
	struct epoll_event evs[10];
	struct mi_audio_play_bridge talkback;
	struct mi_alaw_downsampler audio_ds;
	struct mi_clock_sync clock_sync;
	uint64_t now_us;

	setvbuf(stdout, NULL, _IOLBF, 0);
	setvbuf(stderr, NULL, _IOLBF, 0);

	for (int i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "--audio-rate") == 0 || strcmp(argv[i], "-r") == 0) && i + 1 < argc) {
			int r = atoi(argv[++i]);
			if (r != 8000 && r != 16000) {
				fprintf(stderr, "unsupported audio rate: %d (use 8000 or 16000)\n", r);
				return 1;
			}
			audio_rate = (uint32_t)r;
		} else if ((strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0) && i + 1 < argc) {
			port = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			usage(argv[0]);
			return 0;
		} else if (i == 1 && argv[i][0] != '-') {
			port = atoi(argv[i]);
		} else {
			usage(argv[0]);
			return 1;
		}
	}

	demo = rtsp_new_demo(port);
	if (NULL == demo) {
		printf("rtsp_new_demo failed\n");
		return 1;
	}

	epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (epoll_fd < 0) {
		perror("epoll_create1");
		rtsp_del_demo(demo);
		return 1;
	}

	memset(&clock_sync, 0, sizeof(clock_sync));
	mi_alaw_downsampler_init(&audio_ds);
	mi_audio_play_bridge_init(&talkback, AUDIO_PLAY_PATH);

	now_us = rtsp_get_reltime();
	for (ch = 0; ch < 3; ch++) {
		struct mi_stream_info *p = &mi_info[ch];
		if (!p->enabled) {
			continue;
		}
		if (p->stream_path) {
			uint64_t rel = rtsp_get_reltime();
			uint64_t ntp = rtsp_get_ntptime();
			p->session = rtsp_new_session(demo, p->stream_path);
			if (NULL == p->session) {
				printf("rtsp_new_session %s failed\n", p->stream_path);
				continue;
			}
			rtsp_set_video(p->session, RTSP_CODEC_ID_VIDEO_H265, NULL, 0);
			rtsp_sync_video_ts(p->session, rel, ntp);
			rtsp_set_audio(p->session, RTSP_CODEC_ID_AUDIO_G711A, NULL, 0);
			rtsp_set_audio_sample_rate(p->session, audio_rate);
			rtsp_set_audio_rx_hook(p->session, mi_audio_play_rtsp_rx_hook, &talkback);
			rtsp_sync_audio_ts(p->session, rel, ntp);
		}
		if (connect_stream(epoll_fd, p, now_us) < 0) {
			printf("initial connect %s failed, will retry\n", p->unix_path);
		}
	}

	signal(SIGINT, sig_proc);
	signal(SIGTERM, sig_proc);
	signal(SIGPIPE, SIG_IGN);

	printf("start... (port=%d audio=PCMA/%u)\n", port > 0 ? port : 554, audio_rate);

	while (flag_run) {
		int count = epoll_wait(epoll_fd, evs, 10, EPOLL_WAIT_TIMEOUT_MS);
		if (count < 0) {
			if (errno == EINTR) {
				continue;
			}
			perror("epoll_wait");
			break;
		}

		for (int i = 0; i < count; i++) {
			struct mi_stream_info *stream = evs[i].data.ptr;
			if (evs[i].events & (EPOLLERR | EPOLLHUP)) {
				printf("stream %s epoll hangup/err (0x%x)\n", stream->unix_path, evs[i].events);
				disconnect_stream(epoll_fd, stream);
				continue;
			}
			drain_stream_events(epoll_fd, stream, &clock_sync, &audio_ds, audio_rate);
		}

		do {
			ret = rtsp_do_event(demo);
		} while (ret > 0);
		if (ret < 0) {
			break;
		}

		now_us = rtsp_get_reltime();
		for (ch = 0; ch < 3; ch++) {
			struct mi_stream_info *p = &mi_info[ch];
			if (p->enabled && p->fd < 0 && (now_us - p->last_retry_us) >= RECONNECT_INTERVAL_US) {
				connect_stream(epoll_fd, p, now_us);
			}
		}
	}

	printf("\nexit.........\n");

	for (ch = 0; ch < 3; ch++) {
		struct mi_stream_info *p = &mi_info[ch];
		disconnect_stream(epoll_fd, p);
		if (p->session) {
			rtsp_del_session(p->session);
			p->session = NULL;
		}
	}

	close(epoll_fd);
	mi_audio_play_bridge_close(&talkback);
	rtsp_del_demo(demo);

	return 0;
}

#ifdef __cplusplus
};
#endif
/**************** End Of File: mi_rtsp_server.c *****************/
