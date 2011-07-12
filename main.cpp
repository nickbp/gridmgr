/*
  grid - Organizes windows according to a grid.
  Copyright (C) 2011  Nicholas Parker

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
#include <string.h>
#include <errno.h>

#include "config.h"
#include "grid.h"

using grid::config::error;

void syntax(char* appname) {
	error("");
	error("grid v%s (built %s)",
		  grid::config::VERSION_STRING,
		  grid::config::BUILD_DATE);
	error("Moves/resizes windows to match 2x2/3x3 grid layouts.");
	error("");
	error("Usage: %s [options] <position>", appname);
	error("");
	error("Positions:");
	error("  -----------------------------  ");
	error(" | topleft |  top   | topright | ");
	error(" | ------- + ------ + -------- | ");
	error(" |  left   | center |   right  | ");
	error(" | ------- + ------ + -------- | ");
	error(" | botleft | bottom | botright | ");
	error("  -----------------------------  ");
	error("");
	error("Options:");
	error("  -h/--help        This help text.");
	error("  -v/--verbose     Show verbose output.");
	error("  --log <file>     Append any output to <file>.");
	error("");
	error("Applying the same position to the same window multiple times will");
	error("automatically resize that window to match different grids. Try it!");
	error("");
}

namespace {
	enum CMD { CMD_UNKNOWN, CMD_HELP, CMD_POSITION };
	CMD run_cmd = CMD_UNKNOWN;
	grid::POS position = grid::POS_UNKNOWN;
}

bool parse_config(int argc, char* argv[]) {
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
		c = getopt_long(argc, argv, "hv",
						long_options, &option_index);
		if (c == -1) {//unknown arg (doesnt match -x/--x format)
			if (optind >= argc) {
				//at end of successful parse
				break;
			}
			//getopt refuses to continue, so handle position manually:
			for (int i = optind; i < argc; ++i) {
				const char* arg = argv[i];
				//debug("%d %d %s", argc, i, arg);
				if (strcmp(arg, "topleft") == 0) {
					position = grid::POS_TOP_LEFT;
				} else if (strcmp(arg, "top") == 0) {
					position = grid::POS_TOP_CENTER;
				} else if (strcmp(arg, "topright") == 0) {
					position = grid::POS_TOP_RIGHT;
				} else if (strcmp(arg, "left") == 0) {
					position = grid::POS_LEFT;
				} else if (strcmp(arg, "center") == 0) {
					position = grid::POS_CENTER;
				} else if (strcmp(arg, "right") == 0) {
					position = grid::POS_RIGHT;
				} else if (strcmp(arg, "botleft") == 0) {
					position = grid::POS_BOT_LEFT;
				} else if (strcmp(arg, "bottom") == 0 || strcmp(arg, "bot") == 0) {
					position = grid::POS_BOT_CENTER;
				} else if (strcmp(arg, "botright") == 0) {
					position = grid::POS_BOT_RIGHT;
				} else {
					error("%s: unknown argument: '%s'", argv[0], argv[i]);
					syntax(argv[0]);
					return false;
				}
				if (run_cmd != CMD_UNKNOWN) {
					error("%s: misplaced argument: '%s'", argv[0], argv[i]);
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
			grid::config::debug_enabled = true;
			break;
		case 'l':
			{
				FILE* logfile = fopen(optarg, "a");
				if (logfile == NULL) {
					grid::config::error("Unable to open log file %s: %s", optarg, strerror(errno));
					return false;
				}
				grid::config::fout = logfile;
				grid::config::ferr = logfile;
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
		return 1;
	}
	switch (run_cmd) {
	case CMD_HELP:
		syntax(argv[0]);
		return 0;
	case CMD_POSITION:
		if (position == grid::POS_UNKNOWN) {
			error("INTERNAL ERROR: position command, but position not set!");
			return 1;
		}
		return grid::set_position(position) ? 0 : 1;
	default:
		error("%s: no command specified", argv[0]);
		syntax(argv[0]);
		return 1;
	}
}