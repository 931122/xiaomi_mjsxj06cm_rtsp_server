#include "mi_audio_codec.h"

static int mi_mulaw_to_linear(uint8_t u_val)
{
	int t;

	u_val = (uint8_t)~u_val;
	t = ((u_val & 0x0F) << 3) + 0x84;
	t <<= (u_val & 0x70) >> 4;

	return (u_val & 0x80) ? (0x84 - t) : (t - 0x84);
}

static uint8_t mi_linear_to_alaw(int pcm_val)
{
	static const int seg_end[8] = {0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF};
	int mask;
	int seg;
	uint8_t aval;

	mask = (pcm_val >= 0) ? 0xD5 : 0x55;
	if (pcm_val < 0) {
		pcm_val = -pcm_val - 8;
	}

	if (pcm_val > 0x7FFF) {
		pcm_val = 0x7FFF;
	}

	for (seg = 0; seg < 8; seg++) {
		if (pcm_val <= seg_end[seg]) {
			break;
		}
	}

	if (seg >= 8) {
		return (uint8_t)(0x7F ^ mask);
	}

	aval = (uint8_t)(seg << 4);
	if (seg < 2) {
		aval |= (uint8_t)((pcm_val >> 4) & 0x0F);
	} else {
		aval |= (uint8_t)((pcm_val >> (seg + 3)) & 0x0F);
	}

	return (uint8_t)(aval ^ mask);
}

int mi_downsample_alaw_16k_to_8k(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap)
{
	int i;
	int out_len = 0;

	if (!src || !dst || src_len <= 0 || dst_cap <= 0) {
		return -1;
	}

	for (i = 0; i < src_len && out_len < dst_cap; i += 2) {
		dst[out_len++] = src[i];
	}

	return out_len;
}

int mi_mulaw_to_alaw_buffer(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap)
{
	int i;

	if (!src || !dst || src_len <= 0 || dst_cap < src_len) {
		return -1;
	}

	for (i = 0; i < src_len; i++) {
		dst[i] = mi_linear_to_alaw(mi_mulaw_to_linear(src[i]));
	}

	return src_len;
}
