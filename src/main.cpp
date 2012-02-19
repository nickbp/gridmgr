/*
  gridmgr - Organizes windows according to a grid.
  Copyright (C) 2011-2012  Nicholas Parker

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "config.h"
#include "grid.h"

#define TIMESTR_MAX 128 // arbitrarily large

static void syntax(char* appname) {
	ERROR_RAWDIR("");
	ERROR_RAWDIR("gridmgr v%s (built %s)",
		  config::VERSION_STRING,
		  config::BUILD_DATE);
	ERROR_RAWDIR("");
	ERROR_RAWDIR("Performs one or more of the following, in this order:");
	ERROR_RAWDIR("- Activate a window adjacent to the current window.");
#ifdef USE_XINERAMA
	ERROR_RAWDIR("- Move active window to an adjacent monitor.");
#endif
	ERROR_RAWDIR("- Position active window on a grid layout.");
	ERROR_RAWDIR("");
#ifdef USE_XINERAMA
	ERROR_RAWDIR("Usage: %s [options] <window/monitor/position> [w/m/p] [w/m/p]", appname);
#else
	ERROR_RAWDIR("Usage: %s [options] <window/position> [w/p] [w/p]", appname);
#endif
	ERROR_RAWDIR("");
	ERROR_RAWDIR("Windows (activate adjacent window):");
	ERROR_RAWDIR("          -------           ");
	ERROR_RAWDIR("         |  wup  |          ");
	ERROR_RAWDIR("  ------ + ----- + -------  ");
	ERROR_RAWDIR(" | wleft |       | wright | ");
	ERROR_RAWDIR("  ------ + ----- + -------  ");
	ERROR_RAWDIR("         | wdown |          ");
	ERROR_RAWDIR("          -------           ");
#ifdef USE_XINERAMA
	ERROR_RAWDIR("");
	ERROR_RAWDIR("Monitors (move window to adjacent monitor):");
	ERROR_RAWDIR("          -------           ");
	ERROR_RAWDIR("         |  mup  |          ");
	ERROR_RAWDIR("  ------ + ----- + -------  ");
	ERROR_RAWDIR(" | mleft |       | mright | ");
	ERROR_RAWDIR("  ------ + ----- + -------  ");
	ERROR_RAWDIR("         | mdown |          ");
	ERROR_RAWDIR("          -------           ");
#endif
	ERROR_RAWDIR("");
	ERROR_RAWDIR("Positions (position window on grid):");
	ERROR_RAWDIR("  -----------------------------  ");
	ERROR_RAWDIR(" | topleft |  top   | topright | ");
	ERROR_RAWDIR(" | ------- + ------ + -------- | ");
	ERROR_RAWDIR(" |  left   | center |   right  | ");
	ERROR_RAWDIR(" | ------- + ------ + -------- | ");
	ERROR_RAWDIR(" | botleft | bottom | botright | ");
	ERROR_RAWDIR("  -----------------------------  ");
	ERROR_RAWDIR("");
	ERROR_RAWDIR("Options:");
	ERROR_RAWDIR("  -h/--help        This help text.");
	ERROR_RAWDIR("  -v/--verbose     Show verbose output.");
	ERROR_RAWDIR("  --log <file>     Append any output to <file>.");
	ERROR_RAWDIR("");
}

static bool str_to_pos(const char* arg, grid::POS& out) {
	if (strcmp(arg, "topleft") == 0) {
		out = grid::POS_TOP_LEFT;
	} else if (strcmp(arg, "top") == 0) {
		out = grid::POS_TOP_CENTER;
	} else if (strcmp(arg, "topright") == 0) {
		out = grid::POS_TOP_RIGHT;
	} else if (strcmp(arg, "left") == 0) {
		out = grid::POS_LEFT;
	} else if (strcmp(arg, "center") == 0) {
		out = grid::POS_CENTER;
	} else if (strcmp(arg, "right") == 0) {
		out = grid::POS_RIGHT;
	} else if (strcmp(arg, "botleft") == 0) {
		out = grid::POS_BOT_LEFT;
	} else if (strcmp(arg, "bottom") == 0 || strcmp(arg, "bot") == 0) {
		out = grid::POS_BOT_CENTER;
	} else if (strcmp(arg, "botright") == 0) {
		out = grid::POS_BOT_RIGHT;
	} else {
		return false;
	}
	return true;
}

static bool str_to_win(const char* arg, grid::DIR& out) {
	if (strcmp(arg, "wup") == 0) {
		out = grid::DIR_UP;
	} else if (strcmp(arg, "wleft") == 0) {
		out = grid::DIR_LEFT;
	} else if (strcmp(arg, "wright") == 0) {
		out = grid::DIR_RIGHT;
	} else if (strcmp(arg, "wdown") == 0) {
		out = grid::DIR_DOWN;
	} else {
		return false;
	}
	return true;
}

static bool str_to_mon(const char* arg, grid::DIR& out) {
	if (strcmp(arg, "mup") == 0) {
		out = grid::DIR_UP;
	} else if (strcmp(arg, "mleft") == 0) {
		out = grid::DIR_LEFT;
	} else if (strcmp(arg, "mright") == 0) {
		out = grid::DIR_RIGHT;
	} else if (strcmp(arg, "mdown") == 0) {
		out = grid::DIR_DOWN;
	} else {
		return false;
	}
	return true;
}

namespace {
	enum CMD { CMD_UNKNOWN, CMD_HELP, CMD_POSITION };
	CMD run_cmd = CMD_UNKNOWN;
	grid::POS position = grid::POS_CURRENT;
	grid::DIR monitor = grid::DIR_CURRENT, window = grid::DIR_CURRENT;
}

static bool parse_config(int argc, char* argv[]) {
	if (argc == 1) {
		syntax(argv[0]);
		return false;
	}
	int c;
	while (1) {
		static struct option long_options[] = {
			{"help", 0, NULL, 'h'},
			{"verbose", 0, NULL, 'v'},
			{"log", required_argument, NULL, 'l'},
			{0,0,0,0}
		};

		int option_index = 0;
		c = getopt_long(argc, argv, "hvl",
				long_options, &option_index);
		if (c == -1) {//unknown arg (doesnt match -x/--x format)
			if (optind >= argc) {
				//at end of successful parse
				break;
			}
			//getopt refuses to continue, so handle position manually:
			for (int i = optind; i < argc; ++i) {
				const char* arg = argv[i];
				//DEBUG("%d %d %s", argc, i, arg);
				grid::POS tmp_pos;
				grid::DIR tmp_dir;
				if (str_to_pos(arg, tmp_pos)) {
					if (position != grid::POS_CURRENT) {
						ERROR("%s: Multiple positions specified: '%s'", argv[0], argv[i]);
						syntax(argv[0]);
						return false;
					}
					position = tmp_pos;
				} else if (str_to_win(arg, tmp_dir)) {
					if (window != grid::DIR_CURRENT) {
						ERROR("%s: Multiple windows specified: '%s'", argv[0], argv[i]);
						syntax(argv[0]);
						return false;
					}
					window = tmp_dir;
				} else if (str_to_mon(arg, tmp_dir)) {
					if (monitor != grid::DIR_CURRENT) {
						ERROR("%s: Multiple monitors specified: '%s'", argv[0], argv[i]);
						syntax(argv[0]);
						return false;
					}
					monitor = tmp_dir;
				} else {
					ERROR("%s: Unknown argument: '%s'", argv[0], argv[i]);
					syntax(argv[0]);
					return false;
				}
				run_cmd = CMD_POSITION;
			}
			break;
		}

		switch (c) {
		case 'h':
			run_cmd = CMD_HELP;
			return true;
		case 'v':
			config::debug_enabled = true;
			break;
		case 'l':
			{
				FILE* logfile = fopen(optarg, "a");
				if (logfile == NULL) {
					ERROR("Unable to open log file %s: %s", optarg, strerror(errno));
					return false;
				}
				config::fout = logfile;
				config::ferr = logfile;
				char now_s[TIMESTR_MAX];
				{
					time_t now = time(NULL);
					struct tm now_tm;
					localtime_r(&now, &now_tm);
					if (strftime(now_s, TIMESTR_MAX, "%a, %d %b %Y %T %z", &now_tm) == 0) {
						ERROR_DIR("strftime failed");
						return false;
					}
				}
				fprintf(logfile, "--- %s ---\n", now_s);
				for (int i = 0; i < argc && argc < 100;) {
					fprintf(logfile, "%s", argv[i]);
					if (++i < argc) {
						fprintf(logfile, " ");
					}
				}
				fprintf(logfile, "\n");
			}
			break;
		default:
			syntax(argv[0]);
			return false;
		}
	}

	return true;
}

int main(int argc, char* argv[]) {
	if (!parse_config(argc, argv)) {
		return EXIT_FAILURE;
	}
	switch (run_cmd) {
	case CMD_HELP:
		syntax(argv[0]);
		return EXIT_SUCCESS;
	case CMD_POSITION:
		// activate window (if specified)
		if (window != grid::DIR_CURRENT && !grid::set_window(window)) {
			return EXIT_FAILURE;
		}
		// move window (if specified)
		if (position != grid::POS_CURRENT || monitor != grid::DIR_CURRENT) {
			return grid::set_position(position, monitor) ?
				EXIT_SUCCESS : EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	default:
		ERROR("%s: no command specified", argv[0]);
		syntax(argv[0]);
		return EXIT_FAILURE;
	}
}
