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
	MODE_LAST        = 4
};

struct Options {
	enum Mode mode;
	uint8_t rules[3];
	bool invert;
};

const char* modestr(enum Mode mode);
enum ParseStatus parse_args(struct Options* options, int argc, char* argv[]);

#endif /* OPTIONS_H */
