#ifndef __MI_AUDIO_PLAY_H__
#define __MI_AUDIO_PLAY_H__

#include <stdint.h>

#include "rtsp_demo.h"

#define MI_AUDIO_PLAY_FRAME_SIZE 640

struct mi_audio_play_bridge {
	int fd;
	uint8_t pending[MI_AUDIO_PLAY_FRAME_SIZE];
	int pending_len;
	const char *socket_path;
};

void mi_audio_play_bridge_init(struct mi_audio_play_bridge *bridge, const char *socket_path);
void mi_audio_play_bridge_close(struct mi_audio_play_bridge *bridge);
int mi_audio_play_bridge_push(struct mi_audio_play_bridge *bridge, const uint8_t *frame, int len);
int mi_audio_play_rtsp_rx_hook(void *opaque, const uint8_t *frame, int len, uint32_t timestamp, int payload_type);

#endif
