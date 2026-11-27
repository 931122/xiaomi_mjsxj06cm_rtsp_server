#include <string.h>

#include "mi_audio_codec.h"

#define MI_ALAW_FIR_HALF ((MI_ALAW_FIR_TAPS - 1) / 2)
#define MI_ALAW_CHUNK_SAMPLES 256

/* 21-tap Blackman-windowed low-pass FIR (Fs=16kHz, Fc=3.4kHz, Q15, sum=32768). */
static const int16_t k_fir_lpf_q15[MI_ALAW_FIR_HALF + 1] = {
	6, -20, -101, 19, 467, 331, -1214, -1946, 2069, 9809, 13928
};

static const int16_t k_alaw_to_linear[256] = {
	 -5504,  -5248,  -6016,  -5760,  -4480,  -4224,  -4992,  -4736,
	 -7552,  -7296,  -8064,  -7808,  -6528,  -6272,  -7040,  -6784,
	 -2752,  -2624,  -3008,  -2880,  -2240,  -2112,  -2496,  -2368,
	 -3776,  -3648,  -4032,  -3904,  -3264,  -3136,  -3520,  -3392,
	-22016, -20992, -24064, -23040, -17920, -16896, -19968, -18944,
	-30208, -29184, -32256, -31232, -26112, -25088, -28160, -27136,
	-11008, -10496, -12032, -11520,  -8960,  -8448,  -9984,  -9472,
	-15104, -14592, -16128, -15616, -13056, -12544, -14080, -13568,
	  -344,   -328,   -376,   -360,   -280,   -264,   -312,   -296,
	  -472,   -456,   -504,   -488,   -408,   -392,   -440,   -424,
	   -88,    -72,   -120,   -104,    -24,     -8,    -56,    -40,
	  -216,   -200,   -248,   -232,   -152,   -136,   -184,   -168,
	 -1376,  -1312,  -1504,  -1440,  -1120,  -1056,  -1248,  -1184,
	 -1888,  -1824,  -2016,  -1952,  -1632,  -1568,  -1760,  -1696,
	  -688,   -656,   -752,   -720,   -560,   -528,   -624,   -592,
	  -944,   -912,  -1008,   -976,   -816,   -784,   -880,   -848,
	  5504,   5248,   6016,   5760,   4480,   4224,   4992,   4736,
	  7552,   7296,   8064,   7808,   6528,   6272,   7040,   6784,
	  2752,   2624,   3008,   2880,   2240,   2112,   2496,   2368,
	  3776,   3648,   4032,   3904,   3264,   3136,   3520,   3392,
	 22016,  20992,  24064,  23040,  17920,  16896,  19968,  18944,
	 30208,  29184,  32256,  31232,  26112,  25088,  28160,  27136,
	 11008,  10496,  12032,  11520,   8960,   8448,   9984,   9472,
	 15104,  14592,  16128,  15616,  13056,  12544,  14080,  13568,
	   344,    328,    376,    360,    280,    264,    312,    296,
	   472,    456,    504,    488,    408,    392,    440,    424,
	    88,     72,    120,    104,     24,      8,     56,     40,
	   216,    200,    248,    232,    152,    136,    184,    168,
	  1376,   1312,   1504,   1440,   1120,   1056,   1248,   1184,
	  1888,   1824,   2016,   1952,   1632,   1568,   1760,   1696,
	   688,    656,    752,    720,    560,    528,    624,    592,
	   944,    912,   1008,    976,    816,    784,    880,    848,
};

static const uint8_t k_ulaw_to_alaw[256] = {
	0x2A, 0x2B, 0x28, 0x29, 0x2E, 0x2F, 0x2C, 0x2D, 0x22, 0x23, 0x20, 0x21, 0x26, 0x27, 0x24, 0x25,
	0x3A, 0x3B, 0x38, 0x39, 0x3E, 0x3F, 0x3C, 0x3D, 0x32, 0x33, 0x30, 0x31, 0x36, 0x37, 0x34, 0x35,
	0x0B, 0x08, 0x09, 0x0E, 0x0F, 0x0C, 0x0D, 0x02, 0x03, 0x00, 0x01, 0x06, 0x07, 0x04, 0x05, 0x1A,
	0x1B, 0x18, 0x19, 0x1E, 0x1F, 0x1C, 0x1D, 0x12, 0x13, 0x10, 0x11, 0x16, 0x17, 0x14, 0x15, 0x6B,
	0x68, 0x69, 0x6E, 0x6F, 0x6C, 0x6D, 0x62, 0x63, 0x60, 0x61, 0x66, 0x67, 0x64, 0x65, 0x7B, 0x79,
	0x7E, 0x7F, 0x7C, 0x7D, 0x72, 0x73, 0x70, 0x71, 0x76, 0x77, 0x74, 0x75, 0x4B, 0x49, 0x4F, 0x4D,
	0x42, 0x43, 0x40, 0x41, 0x46, 0x47, 0x44, 0x45, 0x5A, 0x5B, 0x58, 0x59, 0x5E, 0x5F, 0x5C, 0x5D,
	0x52, 0x53, 0x53, 0x50, 0x50, 0x51, 0x51, 0x56, 0x56, 0x57, 0x57, 0x54, 0x54, 0x55, 0x55, 0xD5,
	0xAA, 0xAB, 0xA8, 0xA9, 0xAE, 0xAF, 0xAC, 0xAD, 0xA2, 0xA3, 0xA0, 0xA1, 0xA6, 0xA7, 0xA4, 0xA5,
	0xBA, 0xBB, 0xB8, 0xB9, 0xBE, 0xBF, 0xBC, 0xBD, 0xB2, 0xB3, 0xB0, 0xB1, 0xB6, 0xB7, 0xB4, 0xB5,
	0x8B, 0x88, 0x89, 0x8E, 0x8F, 0x8C, 0x8D, 0x82, 0x83, 0x80, 0x81, 0x86, 0x87, 0x84, 0x85, 0x9A,
	0x9B, 0x98, 0x99, 0x9E, 0x9F, 0x9C, 0x9D, 0x92, 0x93, 0x90, 0x91, 0x96, 0x97, 0x94, 0x95, 0xEB,
	0xE8, 0xE9, 0xEE, 0xEF, 0xEC, 0xED, 0xE2, 0xE3, 0xE0, 0xE1, 0xE6, 0xE7, 0xE4, 0xE5, 0xFB, 0xF9,
	0xFE, 0xFF, 0xFC, 0xFD, 0xF2, 0xF3, 0xF0, 0xF1, 0xF6, 0xF7, 0xF4, 0xF5, 0xCB, 0xC9, 0xCF, 0xCD,
	0xC2, 0xC3, 0xC0, 0xC1, 0xC6, 0xC7, 0xC4, 0xC5, 0xDA, 0xDB, 0xD8, 0xD9, 0xDE, 0xDF, 0xDC, 0xDD,
	0xD2, 0xD2, 0xD3, 0xD3, 0xD0, 0xD0, 0xD1, 0xD1, 0xD6, 0xD6, 0xD7, 0xD7, 0xD4, 0xD4, 0xD5, 0xD5,
};

int16_t mi_alaw_to_linear(uint8_t a_val)
{
	return k_alaw_to_linear[a_val];
}

uint8_t mi_linear_to_alaw(int pcm_val)
{
	static const int seg_end[8] = {
		0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF
	};
	int mask;
	int seg;
	uint8_t aval;

	if (pcm_val >= 0) {
		mask = 0xD5;
	} else {
		mask = 0x55;
		pcm_val = -pcm_val - 1;
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

void mi_alaw_downsampler_init(struct mi_alaw_downsampler *ds)
{
	if (!ds) {
		return;
	}

	memset(ds, 0, sizeof(*ds));
}

int mi_alaw_downsampler_process(struct mi_alaw_downsampler *ds,
				const uint8_t *src, int src_len,
				uint8_t *dst, int dst_cap)
{
	int16_t work[(MI_ALAW_FIR_TAPS - 1) + MI_ALAW_CHUNK_SAMPLES];
	int offset = 0;
	int out_len = 0;

	if (!ds || !src || !dst || src_len <= 0 || dst_cap <= 0) {
		return -1;
	}

	while (offset < src_len && out_len < dst_cap) {
		int chunk = src_len - offset;
		int i;

		if (chunk > MI_ALAW_CHUNK_SAMPLES) {
			chunk = MI_ALAW_CHUNK_SAMPLES;
		}

		memcpy(work, ds->history, sizeof(ds->history));
		for (i = 0; i < chunk; i++) {
			work[(MI_ALAW_FIR_TAPS - 1) + i] = k_alaw_to_linear[src[offset + i]];
		}

		for (i = ds->parity; i < chunk && out_len < dst_cap; i += 2) {
			int32_t acc = (int32_t)work[i + MI_ALAW_FIR_HALF] * k_fir_lpf_q15[MI_ALAW_FIR_HALF];
			int k;

			for (k = 0; k < MI_ALAW_FIR_HALF; k++) {
				int32_t pair = (int32_t)work[i + k] + (int32_t)work[i + (MI_ALAW_FIR_TAPS - 1) - k];
				acc += pair * k_fir_lpf_q15[k];
			}

			acc = (acc + 16384) >> 15;
			if (acc > 32767) {
				acc = 32767;
			} else if (acc < -32768) {
				acc = -32768;
			}
			dst[out_len++] = mi_linear_to_alaw(acc);
		}

		ds->parity = (chunk - ds->parity) & 1;
		memcpy(ds->history, work + chunk, sizeof(ds->history));
		offset += chunk;
	}

	return out_len;
}

int mi_downsample_alaw_16k_to_8k(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap)
{
	struct mi_alaw_downsampler ds;

	mi_alaw_downsampler_init(&ds);
	return mi_alaw_downsampler_process(&ds, src, src_len, dst, dst_cap);
}

void mi_alaw_upsampler_init(struct mi_alaw_upsampler *us)
{
	if (!us) {
		return;
	}

	memset(us, 0, sizeof(*us));
}

int mi_alaw_upsampler_process(struct mi_alaw_upsampler *us,
			      const uint8_t *src, int src_len,
			      uint8_t *dst, int dst_cap)
{
	int i;
	int out_len = 0;

	if (!us || !src || !dst || src_len <= 0 || dst_cap < 2) {
		return -1;
	}

	for (i = 0; i < src_len && out_len + 1 < dst_cap; i++) {
		int16_t curr = k_alaw_to_linear[src[i]];
		int32_t mid;

		if (!us->initialized) {
			us->prev_sample = curr;
			us->initialized = 1;
		}
		mid = ((int32_t)us->prev_sample + (int32_t)curr) >> 1;
		dst[out_len++] = mi_linear_to_alaw((int)mid);
		dst[out_len++] = src[i];
		us->prev_sample = curr;
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
		dst[i] = k_ulaw_to_alaw[src[i]];
	}

	return src_len;
}
