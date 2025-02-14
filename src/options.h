#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>
#include <stdint.h>

extern const char* help_text;

enum ParseStatus {
	PARSE_OK      = 0,
	PARSE_HELP    = 1,
	PARSE_NO_ARG  = 2,
	PARSE_BAD_OPT = 3,
	PARSE_BAD_ARG = 4
};

enum Mode {
	MODE_UNKNOWN     = 0,
	MODE_STANDARD    = 1,
	MODE_SPLIT       = 2,
	MODE_DIRECTIONAL = 3,
	MODE_LIST_RULES  = 4,
	MODE_LAST        = 5
};

enum Initial {
	INIT_UNKNOWN   = 0,
	INIT_STANDARD  = 1,
	INIT_ALTERNATE = 2,
	INIT_RANDOM    = 3,
	INIT_LAST      = 4
};

struct Options {
	enum Mode mode;
	enum Initial initial;
	uint8_t rules[3];
};

const char* modestr(enum Mode mode);
const char* initstr(enum Initial mode);
enum ParseStatus parse_args(struct Options* options, int argc, char* argv[]);

#endif /* OPTIONS_H */
