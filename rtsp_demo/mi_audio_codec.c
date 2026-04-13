#include "mi_audio_codec.h"

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
