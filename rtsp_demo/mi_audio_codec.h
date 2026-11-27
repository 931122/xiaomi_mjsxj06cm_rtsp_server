#ifndef __MI_AUDIO_CODEC_H__
#define __MI_AUDIO_CODEC_H__

#include <stdint.h>

#define MI_ALAW_FIR_TAPS 21

struct mi_alaw_downsampler {
	int16_t history[MI_ALAW_FIR_TAPS - 1];
	int parity;
};

struct mi_alaw_upsampler {
	int16_t prev_sample;
	int initialized;
};

void mi_alaw_downsampler_init(struct mi_alaw_downsampler *ds);
int mi_alaw_downsampler_process(struct mi_alaw_downsampler *ds,
				const uint8_t *src, int src_len,
				uint8_t *dst, int dst_cap);

void mi_alaw_upsampler_init(struct mi_alaw_upsampler *us);
int mi_alaw_upsampler_process(struct mi_alaw_upsampler *us,
			      const uint8_t *src, int src_len,
			      uint8_t *dst, int dst_cap);

int16_t mi_alaw_to_linear(uint8_t a_val);
uint8_t mi_linear_to_alaw(int pcm_val);

int mi_downsample_alaw_16k_to_8k(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap);
int mi_mulaw_to_alaw_buffer(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap);

#endif
