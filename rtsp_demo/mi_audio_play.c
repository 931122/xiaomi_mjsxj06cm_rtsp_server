#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "mi_audio_play.h"
#include "mi_audio_codec.h"
#include "mi_unix.h"

#define MI_AUDIO_PLAY_CHUNK_SIZE 512

static int mi_audio_play_send_once(int fd, struct msghdr *msg)
{
	ssize_t ret = sendmsg(fd, msg, MSG_EOR | MSG_NOSIGNAL);

	if (ret == 4 + MI_AUDIO_PLAY_FRAME_SIZE) {
		return 0;
	}

	if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		struct pollfd pfd;

		pfd.fd = fd;
		pfd.events = POLLOUT;
		pfd.revents = 0;
		if (poll(&pfd, 1, 20) > 0 && (pfd.revents & POLLOUT)) {
			ret = sendmsg(fd, msg, MSG_EOR | MSG_NOSIGNAL);
			if (ret == 4 + MI_AUDIO_PLAY_FRAME_SIZE) {
				return 0;
			}
		}
		return -1;
	}

	return -2;
}

static int mi_audio_play_send_frame(struct mi_audio_play_bridge *bridge, const uint8_t *payload)
{
	struct iovec iov[2];
	struct msghdr msg;
	uint8_t header[4] = {0};
	int rc;

	if (bridge->fd < 0) {
		bridge->fd = mi_unix_sock_connect_seqpacket(bridge->socket_path, 1);
		if (bridge->fd < 0) {
			return -1;
		}
	}

	memset(&msg, 0, sizeof(msg));
	iov[0].iov_base = header;
	iov[0].iov_len = sizeof(header);
	iov[1].iov_base = (void *)payload;
	iov[1].iov_len = MI_AUDIO_PLAY_FRAME_SIZE;
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;

	rc = mi_audio_play_send_once(bridge->fd, &msg);
	if (rc == 0) {
		return 0;
	}
	if (rc == -1) {
		return -1;
	}

	perror("audio_play sendmsg");
	close(bridge->fd);
	bridge->fd = -1;

	bridge->fd = mi_unix_sock_connect_seqpacket(bridge->socket_path, 1);
	if (bridge->fd < 0) {
		return -1;
	}

	if (mi_audio_play_send_once(bridge->fd, &msg) != 0) {
		perror("audio_play resendmsg");
		return -1;
	}

	return 0;
}

void mi_audio_play_bridge_init(struct mi_audio_play_bridge *bridge, const char *socket_path)
{
	if (!bridge) {
		return;
	}

	memset(bridge, 0, sizeof(*bridge));
	bridge->fd = -1;
	bridge->socket_path = socket_path;
	mi_alaw_upsampler_init(&bridge->upsampler);
}

void mi_audio_play_bridge_close(struct mi_audio_play_bridge *bridge)
{
	if (!bridge) {
		return;
	}

	if (bridge->fd >= 0) {
		close(bridge->fd);
		bridge->fd = -1;
	}
}

int mi_audio_play_bridge_push(struct mi_audio_play_bridge *bridge, const uint8_t *frame, int len)
{
	int offset = 0;
	int ret = 0;

	if (!bridge || !frame || len <= 0) {
		return -1;
	}

	while (offset < len) {
		int copy_len = MI_AUDIO_PLAY_FRAME_SIZE - bridge->pending_len;
		if (copy_len > len - offset) {
			copy_len = len - offset;
		}

		memcpy(bridge->pending + bridge->pending_len, frame + offset, copy_len);
		bridge->pending_len += copy_len;
		offset += copy_len;

		if (bridge->pending_len == MI_AUDIO_PLAY_FRAME_SIZE) {
			if (mi_audio_play_send_frame(bridge, bridge->pending) < 0) {
				ret = -1;
			}
			bridge->pending_len = 0;
		}
	}

	return ret;
}

int mi_audio_play_rtsp_rx_hook(void *opaque, const uint8_t *frame, int len, uint32_t timestamp, int payload_type)
{
	struct mi_audio_play_bridge *bridge = (struct mi_audio_play_bridge *)opaque;

	(void)timestamp;

	if (!bridge || !frame || len <= 0) {
		return -1;
	}

	/*
	 * Standard static RTP payload types 0 (PCMU/8000) and 8 (PCMA/8000)
	 * are used by Synology Surveillance Station, go2rtc, and ONVIF NVRs.
	 * The Xiaomi device audio_play node expects 16kHz A-law frames, so
	 * transcode PCMU -> PCMA and upsample 8kHz -> 16kHz on ingress.
	 * Dynamic payload types (>= 96, e.g. talkback_push.sh PCMA/16000) pass through directly.
	 */
	if (payload_type == 0 || payload_type == 8) {
		uint8_t alaw8k[MI_AUDIO_PLAY_CHUNK_SIZE];
		uint8_t alaw16k[MI_AUDIO_PLAY_CHUNK_SIZE * 2];
		int offset = 0;

		while (offset < len) {
			int chunk = len - offset;
			const uint8_t *src_8k = frame + offset;
			int up_len;

			if (chunk > MI_AUDIO_PLAY_CHUNK_SIZE) {
				chunk = MI_AUDIO_PLAY_CHUNK_SIZE;
			}
			if (payload_type == 0) {
				if (mi_mulaw_to_alaw_buffer(frame + offset, chunk, alaw8k, sizeof(alaw8k)) <= 0) {
					return -1;
				}
				src_8k = alaw8k;
			}
			up_len = mi_alaw_upsampler_process(&bridge->upsampler, src_8k, chunk, alaw16k, sizeof(alaw16k));
			if (up_len <= 0) {
				return -1;
			}
			if (mi_audio_play_bridge_push(bridge, alaw16k, up_len) < 0) {
				return -1;
			}
			offset += chunk;
		}
		return 0;
	}

	return mi_audio_play_bridge_push(bridge, frame, len);
}
