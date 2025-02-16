#ifndef ECA_H
#define ECA_H

#include <stdint.h>
#include <stddef.h>

extern uint8_t pixel_off;
extern uint8_t pixel_half;
extern uint8_t pixel_on;

/**
 * The "mirror" of a rule is reflected horizontally.
 */
uint8_t get_mirror_rule(uint8_t r);

/**
 * The "complement" of a rule is.
 */
uint8_t get_complement_rule(uint8_t r);

/* populates an initial generation */
typedef void eca_init_fn(uint8_t* dst, size_t width, int channel_count);

/**
 * Only the centre cell is activated.
 */
void eca_initialise(uint8_t* dst, size_t width, int channel_count);

/**
 * Every other cell is activated.
 */
void eca_initialise_alternate(uint8_t* dst, size_t width, int channel_count);

/**
 * Cells are activated at "random", each generation will be identical.
 */
void eca_initialise_random(uint8_t* dst, size_t width, int channel_count);

/* generates the next generation */
typedef void eca_gen_fn(
	uint8_t* dst, const uint8_t* src, size_t width, int channel_count,
	uint8_t rules[channel_count]
);

/**
 * Standard generation.
 *
 * - only the first rule provided is used.
 */
void eca_generate(
	uint8_t* dst, const uint8_t* src, size_t width, int channel_count,
	uint8_t rules[channel_count]
);

/**
 * Each channel is treated individially.
 */
void eca_generate_split(
	uint8_t* dst, const uint8_t* src, size_t width, int channel_count,
	uint8_t rules[channel_count]
);

/**
 * Cells are coloured depending on which parents are activated.
 *
 * - `channel_count` must be 3.
 * - only the first rule provided is used.
 */
void eca_generate_directional(
	uint8_t* dst, const uint8_t* src, size_t width, int channel_count,
	uint8_t rules[channel_count]
);

#endif /* ECA_H */
