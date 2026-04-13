#ifndef __MI_AUDIO_CODEC_H__
#define __MI_AUDIO_CODEC_H__

#include <stdint.h>

int mi_downsample_alaw_16k_to_8k(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap);

#endif
