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
"  -h                    Display this text and exit.\n"
"\n"
"\n"
"Generation Modes (-m):\n"
"  standard              A standard black/white generation.\n"
"  split                 Red, green, and blue channels are split.\n"
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
	enum ParseStatus rv = PARSE_OK;

	long rule_0 = -1;
	long rule_1 = -1;
	long rule_2 = -1;

	options->mode = MODE_STANDARD;
	/* the standard display draws black pixels on a white background */
	options->invert = true;

	int c = -1;
	while ((c = getopt(argc, argv, "him:r:g:b:")) != -1) {
		switch (c) {
			case 'm': {
				options->mode = parse_mode(optarg);
				break;
			}
			case 'r': {
				rule_0 = parse_num(optarg);
				break;
			}
			case 'g': {
				rule_1 = parse_num(optarg);
				break;
			}
			case 'b': {
				rule_2 = parse_num(optarg);
				break;
			}
			case 'i': {
				options->invert = !options->invert;
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

	if (rule_0 < 0 || rule_0 > 255) {
		printf("%s: invalid argument for option -- 'r'\n", argv[0]);
		printf("    range (0, 255)\n");
		rv = PARSE_BAD_ARG;
		goto abort;
	}

	options->rules[0] = rule_0;

	if (options->mode == MODE_SPLIT) {
		if (rule_1 == -1) {
			printf("%s: missing argument for option -- 'g'\n", argv[0]);
			rv = PARSE_NO_ARG;
			goto abort;
		}

		if (rule_1 < 0 || rule_1 > 255) {
			printf("%s: invalid argument for option -- 'g'\n", argv[0]);
			printf("    range (0, 255)\n");
			rv = PARSE_BAD_ARG;
			goto abort;
		}

		if (rule_2 == -1) {
			printf("%s: missing argument for option -- 'b'\n", argv[0]);
			rv = PARSE_NO_ARG;
			goto abort;
		}

		if (rule_2 < 0 || rule_2 > 255) {
			printf("%s: invalid argument for option -- 'b'\n", argv[0]);
			printf("    range (0, 255)\n");
			rv = PARSE_BAD_ARG;
			goto abort;
		}

		options->rules[1] = rule_1;
		options->rules[2] = rule_2;
	}

abort:
	return rv;
}
