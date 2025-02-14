#include "options.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static const char* modestrings[] = {
	"unknown",
	"standard",
	"split",
	"directional"
};

const char* help_text = (
"Usage: wolfram -v -r RULE\n"
"Usage: wolfram [-i] [-m standard]   -r RULE\n"
"Usage: wolfram [-i]  -m directional -r RULE\n"
"Usage: wolfram [-i]  -m split       -r RULE -g RULE -b RULE\n"
"\n"
"Generates an elementary cellular automata.\n"
"\n"
"  -r RULE               Wolfram Rule (0-255)\n"
"  -m MODE               Generation mode {standard, split, directional}\n"
"                        Default: standard\n"
"                        If 'split' is chosen for the mode, both of '-g' and\n"
"                        '-b' must also be specified.\n"
"  -i                    Invert 'on' and 'off' values.\n"
"\n"
"  -g RULE, -b RULE      Specify additional rules for the green and blue\n"
"                        channels. Ignored if the MODE (-m) is not 'split'.\n"
"\n"
"  -v                    Display rule variants (mirror, inverse) and exit.\n"
"  -h                    Display this text and exit.\n"
"\n"
"\n"
"Generation Modes (-m):\n"
"  standard              A standard black/white generation.\n"
"  split                 Red, green, and blue channels are split.\n"
"  directional           The colour of each cell depends on which parents\n"
"                        were responsible for its activation.\n"
);

const char* modestr(enum Mode mode) {
	if (mode >= MODE_LAST) {
		mode = MODE_UNKNOWN;
	}

	return modestrings[mode];
}

bool compare(const char* a, const char* b) {
	size_t sz_a = strlen(a);
	size_t sz_b = strlen(b);

	if (sz_a != sz_b) {
		return false;
	}

	return memcmp(a, b, sz_a) == 0;
}

enum Mode parse_mode(const char* src) {
	if (compare(src, "standard"))    { return MODE_STANDARD; }
	if (compare(src, "split"))       { return MODE_SPLIT; }
	if (compare(src, "directional")) { return MODE_DIRECTIONAL; }

	return MODE_UNKNOWN;
}

long parse_num(const char* src) {
	const char* strend = src + strlen(src);
	char* endptr = NULL;
	long num = strtol(src, &endptr, 10);

	if (src == endptr) {
		return -2;
	}
	if (strend != endptr) {
		return -3;
	}

	return num;
}

enum ParseStatus parse_args(struct Options* options, int argc, char* argv[]) {
	/* todo: mutually exclusive options? no -m and -v? */
	enum ParseStatus rv = PARSE_OK;

	bool r_set = false;
	bool g_set = false;
	bool b_set = false;

	long r_value = 0;
	long g_value = 0;
	long b_value = 0;

	options->mode = MODE_STANDARD;

	int c = -1;
	while ((c = getopt(argc, argv, "hvim:r:g:b:")) != -1) {
		switch (c) {
			case 'm': {
				options->mode = parse_mode(optarg);
				break;
			}
			case 'v': {
				options->mode = MODE_LIST_RULES;
				break;
			}
			case 'r': {
				r_set = true;
				r_value = parse_num(optarg);
				break;
			}
			case 'g': {
				g_set = true;
				g_value = parse_num(optarg);
				break;
			}
			case 'b': {
				b_set = true;
				b_value = parse_num(optarg);
				break;
			}
			case 'i': {
				options->invert = true;
				break;
			}
			case 'h': rv = PARSE_HELP; goto abort;
			case ':': rv = PARSE_NO_ARG; goto abort;
			case '?': rv = PARSE_BAD_OPT; goto abort;
		}
	}

	if (options->mode == MODE_UNKNOWN) {
		printf("%s: invalid argument for option -- 'm'\n", argv[0]);
		printf("    choice {standard, split, directional}\n");
		rv = PARSE_BAD_ARG;
		goto abort;
	}

	if (options->mode == MODE_STANDARD) {
		/* standard display draws black pixels on a white background */
		options->invert = !options->invert;
	}

	if (!r_set) {
		printf("%s: missing option -- 'r'\n", argv[0]);
		rv = PARSE_NO_ARG;
		goto abort;
	}
	if (r_value < 0 || r_value > 255) {
		printf("%s: rule out of range -- 'r'\n", argv[0]);
		rv = PARSE_BAD_ARG;
		goto abort;
	}
	options->rules[0] = r_value;

	if (options->mode == MODE_SPLIT) {
		if (!g_set) {
			printf("%s: missing option -- 'g'\n", argv[0]);
			rv = PARSE_NO_ARG;
			goto abort;
		}
		if (g_value < 0 || g_value > 255) {
			printf("%s: rule out of range -- 'g'\n", argv[0]);
			rv = PARSE_BAD_ARG;
			goto abort;
		}
		options->rules[1] = g_value;

		if (!b_set) {
			printf("%s: missing option -- 'b'\n", argv[0]);
			rv = PARSE_NO_ARG;
			goto abort;
		}
		if (b_value < 0 || b_value > 255) {
			printf("%s: rule out of range -- 'b'\n", argv[0]);
			rv = PARSE_BAD_ARG;
			goto abort;
		}
		options->rules[2] = b_value;
	}

abort:
	return rv;
}
