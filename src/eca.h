#ifndef ECA_H
#define ECA_H

#include <stdint.h>
#include <stddef.h>

extern uint8_t pixel_off;
extern uint8_t pixel_half;
extern uint8_t pixel_on;

uint8_t get_mirror_rule(uint8_t r);
uint8_t get_complement_rule(uint8_t r);

void eca_generate(
	uint8_t* dst, const uint8_t* src, size_t width, uint8_t rule
);

void eca_generate_split(
	uint8_t* dst, const uint8_t* src, size_t width, uint8_t rules[3]
);

void eca_generate_directional(
	uint8_t* dst, const uint8_t* src, size_t width, uint8_t rule
);

#endif /* ECA_H */
