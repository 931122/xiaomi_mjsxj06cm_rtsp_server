#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "mi_audio_play.h"
#include "mi_unix.h"

static int mi_audio_play_send_frame(struct mi_audio_play_bridge *bridge, const uint8_t *payload)
{
	struct iovec iov[2];
	struct msghdr msg;
	uint8_t header[4] = {0};
	ssize_t ret;

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

	ret = sendmsg(bridge->fd, &msg, MSG_EOR | MSG_NOSIGNAL);
	if (ret == 4 + MI_AUDIO_PLAY_FRAME_SIZE) {
		return 0;
	}

	if (ret < 0) {
		perror("audio_play sendmsg");
	} else {
		fprintf(stderr, "audio_play short send: %zd\n", ret);
	}

	close(bridge->fd);
	bridge->fd = -1;

	bridge->fd = mi_unix_sock_connect_seqpacket(bridge->socket_path, 1);
	if (bridge->fd < 0) {
		return -1;
	}

	ret = sendmsg(bridge->fd, &msg, MSG_EOR | MSG_NOSIGNAL);
	if (ret != 4 + MI_AUDIO_PLAY_FRAME_SIZE) {
		if (ret < 0) {
			perror("audio_play resendmsg");
		} else {
			fprintf(stderr, "audio_play resend short send: %zd\n", ret);
		}
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
			mi_audio_play_send_frame(bridge, bridge->pending);
			bridge->pending_len = 0;
		}
	}

	return 0;
}

int mi_audio_play_rtsp_rx_hook(void *opaque, const uint8_t *frame, int len, uint32_t timestamp, int payload_type)
{
	struct mi_audio_play_bridge *bridge = (struct mi_audio_play_bridge *)opaque;

	(void)timestamp;
	(void)payload_type;

	return mi_audio_play_bridge_push(bridge, frame, len);
}
