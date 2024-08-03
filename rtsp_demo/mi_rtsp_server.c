/********************************************************************
* file: mi_rtsp_server.c               date: 六 2024-08-03 11:00:16 *
*                                                                   *
* Description:                                                      *
*                                                                   *
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>
#include "rtsp_demo.h"

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#define MAX_BUFFER_SIZE 128
#define EPOLL_TIMEOUT 60000 // milliseconds
#define MAX_SESSION_NUM 64


typedef int(*set_frame)(rtsp_session_handle session, const uint8_t *frame, int len, uint64_t ts);

struct mi_stream_info {
	const char *unix_path;
	const char *stream_path;
	const set_frame callback;
	const int msg_offset;
	bool enabled;
	int fd;
	int ch;
	struct epoll_event ev;
	rtsp_session_handle session;
} mi_info[3] = {
	{ "/run/video_mainstream/control", "/main", rtsp_tx_video, 4, true },
	{ "/run/video_substream/control", "/sub", rtsp_tx_video, 4, true },
	{ "/var/run/audio_in/control", NULL, rtsp_tx_audio, 6, true}
};

static int flag_run = 1;
static void sig_proc(int signo)
{
	flag_run = 0;
}

int unix_sock_connect(const char *path)
{
	struct sockaddr_un unixAddr;

	int unixSock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	if (unixSock < 0) {
		perror("socket\n");
		return unixSock;
	}

	memset(&unixAddr, 0, sizeof(unixAddr));
	unixAddr.sun_family = AF_UNIX;
	strcpy(unixAddr.sun_path, path);

	int ret = connect(unixSock, (struct sockaddr*)&unixAddr, sizeof(unixAddr));
	if (ret < 0) {
		perror("connect\n");
		close(unixSock);
		return -1;
	}

	return unixSock;
}

int main(int argc, char *argv[]) {
	int map_len, count;
	int map_fd;
	uint8_t *map_addr;
	struct mi_stream_info* p;
	int epoll_fd;
	int ret, ch = 0;
	uint64_t ts = 0;
	int port = 0;
	unsigned int msg_len;
	rtsp_demo_handle demo;
	struct epoll_event evs[10];
	struct msghdr msg;
	struct cmsghdr cm;
	struct iovec iov[2];
	char iovPart1[4];
	char iovPart2[MAX_BUFFER_SIZE];

	iov[0].iov_base = iovPart1;
	iov[0].iov_len = sizeof(iovPart1);
	iov[1].iov_base = iovPart2;
	iov[1].iov_len = sizeof(iovPart2);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;

	if (argc > 1) {
		port = atoi(argv[1]);
	}

	demo = rtsp_new_demo(port);
	if (NULL == demo) {
		printf("rtsp_new_demo failed\n");
	}

	epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (epoll_fd < 0) {
		perror("epoll_create1");
	}

	for (ch = 0; ch < 3; ch++) {
		p = &mi_info[ch];
		if (!p->enabled) {
			continue;
		}
		p->ch = ch;
		p->fd = unix_sock_connect(p->unix_path);
		if (p->fd < 0) {
			printf("connect %s error\n", p->unix_path);
		}
		recvmsg(p->fd, &msg, MSG_DONTWAIT); /* need recv one package */
		msg.msg_control = &cm;
		msg.msg_controllen = CMSG_LEN(sizeof(int));
		p->ev.events = EPOLLIN;
		p->ev.data.ptr = p;
		ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, p->fd, &p->ev);
		if (ret < 0) {
			printf("epoll add %s error\n", p->unix_path);
		}
		if (NULL == p->stream_path) {
			continue;
		}
		p->session = rtsp_new_session(demo, p->stream_path);
		if (NULL == p->session) {
			printf("rtsp_new_session failed\n");
			continue;
		}
		rtsp_set_video(p->session, RTSP_CODEC_ID_VIDEO_H265, NULL, 0);
		rtsp_sync_video_ts(p->session, rtsp_get_reltime(), rtsp_get_ntptime());
		rtsp_set_audio(p->session, RTSP_CODEC_ID_AUDIO_G711A, NULL, 0);
		rtsp_sync_audio_ts(p->session, rtsp_get_reltime(), rtsp_get_ntptime());
		//rtsp_set_auth(p->session, RTSP_AUTH_TYPE_BASIC, "admin", "123456");
	}
	mi_info[2].session = mi_info[0].session;

	signal(SIGINT, sig_proc);
	signal(SIGPIPE, SIG_IGN);

	printf("start...\n");

	ts = rtsp_get_reltime();
	while (flag_run) {
		count = epoll_wait(epoll_fd, evs, 10, EPOLL_TIMEOUT);
		for (int i = 0; i < count; i++) {
			struct mi_stream_info* p = evs[i].data.ptr;
			ssize_t len = recvmsg(p->fd, &msg, MSG_DONTWAIT);
			if (len < 0) {
				printf("recv err:%d\n", len);
				continue;
			}

			map_len = *((int*)(&iovPart2[4]));
			map_fd  = *(int*)CMSG_DATA(&cm);
			if (unlikely(map_fd <= 0)) {
				printf("map_fd error:%d\n", map_fd);
				continue;
			}

			map_addr = mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_PRIVATE, map_fd, 0);
			if (unlikely((void *)-1 == map_addr)) {
				close(map_fd);
				printf("map error\n");
				continue;
			}

			msg_len = ((unsigned int *)(map_addr + 64))[p->msg_offset];
#if 0
			printf("type: %s, msglen: 0x%x\n", p->stream_path, msg_len);
			for (int i = 0; i < 1024; i++) {
				printf("%02x ", map_addr[i]);
			}
			printf("\n====================\n");
#endif
			if (unlikely(msg_len < 32)) {
				printf("msg too short\n");
				continue;
			}
			else {
				if (likely(p->session)) {
					//(p->callback)(p->session, (const uint8_t *)(map_addr+96), msg_len == 0x280 ? msg_len/2 : msg_len, ts);
					if (!p->stream_path) { /* audio only send main */
						(p->callback)(p->session, (const uint8_t *)(map_addr+96 +320), msg_len/2, ts);
					}
					else {
						(p->callback)(p->session, (const uint8_t *)(map_addr+96), msg_len, ts);
					}
				}
			}

			munmap(map_addr, map_len);
			close(map_fd);
		}

		do {
			ret = rtsp_do_event(demo);
			if (ret > 0)
				continue;
			if (ret < 0)
				break;
			usleep(20000);
		} while (rtsp_get_reltime() - ts < 1000000 / 25);
		if (ret < 0)
			break;
		ts += 1000000 / 25;
	}

	printf("\nexit.........\n");

	for (ch = 0; ch < 3; ch++) {
		p = &mi_info[ch];
		if (!p->enabled || p->fd < 0) {
			continue;
		}
		close(p->fd);
		if (p->session)
			rtsp_del_session(p->session);
	}

	close(epoll_fd);
	rtsp_del_demo(demo);

	return 0;
}





#ifdef __cplusplus
};
#endif
/**************** End Of File: mi_rtsp_server.c *****************/
