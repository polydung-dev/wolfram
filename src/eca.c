#include "eca.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

uint8_t pixel_off  = 0x00;
uint8_t pixel_half = 0x45;
uint8_t pixel_on   = 0xff;

uint8_t get_mirror_rule(uint8_t r) {
	uint8_t nr = r & 0xa5; /* 0b10100101 already mirrored */

	/* swap 3 and 6 */
	nr |= ((r >> 3) & 1) << 6;
	nr |= ((r >> 6) & 1) << 3;

	/* swap 4 and 1 */
	nr |= ((r >> 4) & 1) << 1;
	nr |= ((r >> 1) & 1) << 4;

	return nr;
}

uint8_t get_complement_rule(uint8_t r) {
	uint8_t nr = 0;

	for (int i = 0; i < 8; ++i) {
		nr |= ((r >> i) & 1) << (7 - i);
	}

	return ~nr;
}

/* populates an initial generation */
void eca_initialise(uint8_t* dst, size_t width, int channel_count) {
	size_t row_size = width * channel_count;
	memset(dst, pixel_off, row_size);

	size_t centre_pixel = (width / 2) * channel_count;
	memset(dst + centre_pixel, pixel_on, channel_count);
}


void eca_initialise_alternate(uint8_t* dst, size_t width, int channel_count) {
	size_t row_size = width * channel_count;
	memset(dst, pixel_off, row_size);

	for (size_t i = 0; i < width; ++i) {
		uint8_t val = (i % 2) ? pixel_on : pixel_off;
		memset(dst + (i * channel_count), val, channel_count);
	}
}

void eca_initialise_random(uint8_t* dst, size_t width, int channel_count) {
	size_t row_size = width * channel_count;
	memset(dst, pixel_off, row_size);

	srand(0);
	for (size_t i = 0; i < width; ++i) {
		uint8_t val = (rand() % 2) ? pixel_on : pixel_off;
		memset(dst + (i * channel_count), val, channel_count);
	}
}

/* generates the next generation */
void eca_generate(
	uint8_t* dst, const uint8_t* src, size_t width, int channel_count,
	uint8_t rules[channel_count]
) {
	size_t last_pixel_index = (width - 1) * channel_count;
	for (size_t i = 0; i < width; ++i) {
		size_t pixel_index = i * channel_count;
		size_t left_pixel  = pixel_index - channel_count;
		size_t right_pixel = pixel_index + channel_count;

		/* wrap around edges */
		if (pixel_index == 0) {
			left_pixel = last_pixel_index;
		} else if (pixel_index == last_pixel_index) {
			right_pixel = 0;
		}

		/* convert to rule index */
		char rule_index = 0;
		if (src[left_pixel] == pixel_on) {
			rule_index |= 4;
		}
		if (src[pixel_index] == pixel_on) {
			rule_index |= 2;
		}
		if (src[right_pixel] == pixel_on) {
			rule_index |= 1;
		}

		bool fill_pixel = ((rules[0] >> rule_index) & 1) == 1;
		if (fill_pixel) {
			memset(dst + pixel_index, pixel_on, channel_count);
		}
	}
}

void eca_generate_split(
	uint8_t* dst, const uint8_t* src, size_t width, int channel_count,
	uint8_t rules[channel_count]
) {
	size_t last_pixel_index = (width - 1) * channel_count;
	for (size_t i = 0; i < width; ++i) {
		size_t pixel_index = i * channel_count;
		size_t left_pixel  = pixel_index - channel_count;
		size_t right_pixel = pixel_index + channel_count;

		/* wrap around edges */
		if (pixel_index == 0) {
			left_pixel = last_pixel_index;
		} else if (pixel_index == last_pixel_index) {
			right_pixel = 0;
		}

		/* convert to rule index */
		for (int channel = 0; channel < 3; ++channel) {
			uint8_t rule_index = 0;
			if (src[left_pixel + channel] == pixel_on) {
				rule_index |= 4;
			}
			if (src[pixel_index + channel] == pixel_on) {
				rule_index |= 2;
			}
			if (src[right_pixel + channel] == pixel_on) {
				rule_index |= 1;
			}

			bool fill_pixel = \
				(rules[channel] >> rule_index) & 1;
			dst[pixel_index + channel] = \
				fill_pixel ? pixel_on : pixel_off;
		}

	}
}

void eca_generate_directional(
	uint8_t* dst, const uint8_t* src, size_t width, int channel_count,
	uint8_t rules[channel_count]
) {
	size_t last_pixel_index = (width - 1) * channel_count;
	for (size_t i = 0; i < width; ++i) {
		size_t pixel_index = i * channel_count;
		size_t left_pixel  = pixel_index - channel_count;
		size_t right_pixel = pixel_index + channel_count;

		/* wrap around edges */
		if (pixel_index == 0) {
			left_pixel = last_pixel_index;
		} else if (pixel_index == last_pixel_index) {
			right_pixel = 0;
		}

		bool pixel_set = false;
		bool left_set  = false;
		bool right_set = false;

		for (int channel = 0; channel < channel_count; ++channel) {
			if (src[left_pixel + channel] != pixel_off) {
				left_set = true;
			}
			if (src[pixel_index + channel] != pixel_off) {
				pixel_set = true;
			}
			if (src[right_pixel + channel] != pixel_off) {
				right_set = true;
			}
		}

		/* convert to rule index */
		uint8_t rule_index = 0;
		if (left_set) {
			rule_index |= 4;
		}
		if (pixel_set) {
			rule_index |= 2;
		}
		if (right_set) {
			rule_index |= 1;
		}

		bool fill_pixel = ((rules[0] >> rule_index) & 1) == 1;
		if (fill_pixel) {
			dst[pixel_index  ] = left_set  ? pixel_on : pixel_half;
			dst[pixel_index+1] = pixel_set ? pixel_on : pixel_half;
			dst[pixel_index+2] = right_set ? pixel_on : pixel_half;
		}
	}
}
