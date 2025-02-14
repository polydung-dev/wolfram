#ifndef ECA_H
#define ECA_H

#include <stdint.h>
#include <stddef.h>

extern uint8_t pixel_off;
extern uint8_t pixel_half;
extern uint8_t pixel_on;

uint8_t get_mirror_rule(uint8_t r);
uint8_t get_complement_rule(uint8_t r);

/* populates an initial generation */
typedef void eca_init_fn(
	uint8_t* dst, size_t width, size_t channel_count
);

void eca_initialise(
	uint8_t* dst, size_t width, size_t channel_count
);

void eca_initialise_alternate(
	uint8_t* dst, size_t width, size_t channel_count
);

void eca_initialise_random(
	uint8_t* dst, size_t width, size_t channel_count
);

/* generates the next generation */
typedef void eca_gen_fn(
	uint8_t* dst, const uint8_t* src, size_t width, uint8_t rules[3]
);

void eca_generate(
	uint8_t* dst, const uint8_t* src, size_t width, uint8_t rules[3]
);

void eca_generate_split(
	uint8_t* dst, const uint8_t* src, size_t width, uint8_t rules[3]
);

void eca_generate_directional(
	uint8_t* dst, const uint8_t* src, size_t width, uint8_t rules[3]
);

#endif /* ECA_H */
