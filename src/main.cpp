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
    PRINT_HELP("");
    PRINT_HELP("gridmgr v%s (built %s)",
          config::VERSION_STRING,
          config::BUILD_DATE);
    PRINT_HELP("");
    PRINT_HELP("Performs one or more of the following, in this order:");
    PRINT_HELP("- Activate a window adjacent to the current window.");
#ifdef USE_XINERAMA
    PRINT_HELP("- Move active window to an adjacent monitor.");
#endif
    PRINT_HELP("- Position active window on a grid layout.");
    PRINT_HELP("");
#ifdef USE_XINERAMA
    PRINT_HELP("Usage: %s [options] <window/monitor/position> [w/m/p] [w/m/p]", appname);
#else
    PRINT_HELP("Usage: %s [options] <window/position> [w/p] [w/p]", appname);
#endif
    PRINT_HELP("");
    PRINT_HELP("Windows (activate adjacent (w)indow):");
    PRINT_HELP("  -------- --------- ---------  ");
    PRINT_HELP(" | wuleft |   wup   | wuright | ");
    PRINT_HELP(" |------- + ------- + --------| ");
    PRINT_HELP(" | wleft  |         |  wright | ");
    PRINT_HELP(" |------- + ------- + --------| ");
    PRINT_HELP(" | wdleft |  wdown  | wdright | ");
    PRINT_HELP("  -------- --------- ---------  ");
#ifdef USE_XINERAMA
    PRINT_HELP("");
    PRINT_HELP("Monitors (move window to adjacent (m)onitor):");
    PRINT_HELP("  -------- --------- ---------  ");
    PRINT_HELP(" | muleft |   mup   | muright | ");
    PRINT_HELP(" |------- + ------- + --------| ");
    PRINT_HELP(" | mleft  |         |  mright | ");
    PRINT_HELP(" |------- + ------- + --------| ");
    PRINT_HELP(" | mdleft |  mdown  | mdright | ");
    PRINT_HELP("  -------- --------- ---------  ");
#endif
    PRINT_HELP("");
    PRINT_HELP("Positions (position window on (g)rid):");
    PRINT_HELP("  -------- --------- ---------  ");
    PRINT_HELP(" | guleft |   gup   | guright | ");
    PRINT_HELP(" |------- + ------- + --------| ");
    PRINT_HELP(" | gleft  | gcenter |  gright | ");
    PRINT_HELP(" |------- + ------- + --------| ");
    PRINT_HELP(" | gdleft |  gdown  | gdright | ");
    PRINT_HELP("  -------- --------- ---------  ");
    PRINT_HELP("");
    PRINT_HELP("Options:");
    PRINT_HELP("  -h/--help        This help text.");
    PRINT_HELP("  -v/--verbose     Show verbose output.");
    PRINT_HELP("  --log <file>     Append any output to <file>.");
    PRINT_HELP("");
}

static bool strsub_to_pos(const char* arg, grid::POS& out, bool center_is_valid) {
    if (strcmp(arg, "uleft") == 0) {
        out = grid::POS_UP_LEFT;
    } else if (strcmp(arg, "up") == 0) {
        out = grid::POS_UP_CENTER;
    } else if (strcmp(arg, "uright") == 0) {
        out = grid::POS_UP_RIGHT;

    } else if (strcmp(arg, "left") == 0) {
        out = grid::POS_LEFT;
    } else if (center_is_valid && strcmp(arg, "center") == 0) {
        out = grid::POS_CENTER;
    } else if (strcmp(arg, "right") == 0) {
        out = grid::POS_RIGHT;

    } else if (strcmp(arg, "dleft") == 0) {
        out = grid::POS_DOWN_LEFT;
    } else if (strcmp(arg, "down") == 0) {
        out = grid::POS_DOWN_CENTER;
    } else if (strcmp(arg, "dright") == 0) {
        out = grid::POS_DOWN_RIGHT;

    } else {
        return false;
    }
    return true;
}

namespace {
    enum CMD { CMD_UNKNOWN, CMD_HELP, CMD_POSITION };
    CMD run_cmd = CMD_UNKNOWN;
    grid::POS gridpos = grid::POS_CURRENT,
        monitor = grid::POS_CURRENT, window = grid::POS_CURRENT;
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
                if (arg[0] == 'g' && strsub_to_pos(arg+1, tmp_pos, true)) {
                    if (gridpos != grid::POS_CURRENT) {
                        ERROR("%s: Multiple positions specified: '%s'", argv[0], argv[i]);
                        syntax(argv[0]);
                        return false;
                    }
                    gridpos = tmp_pos;
                } else if (arg[0] == 'w' && strsub_to_pos(arg+1, tmp_pos, false)) {
                    if (window != grid::POS_CURRENT) {
                        ERROR("%s: Multiple windows specified: '%s'", argv[0], argv[i]);
                        syntax(argv[0]);
                        return false;
                    }
                    window = tmp_pos;
                } else if (arg[0] == 'm' && strsub_to_pos(arg+1, tmp_pos, false)) {
                    if (monitor != grid::POS_CURRENT) {
                        ERROR("%s: Multiple monitors specified: '%s'", argv[0], argv[i]);
                        syntax(argv[0]);
                        return false;
                    }
                    monitor = tmp_pos;
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
                        ERROR("strftime failed");
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
        if (window != grid::POS_CURRENT && !grid::set_active(window)) {
            return EXIT_FAILURE;
        }
        // move window (if specified)
        if (gridpos != grid::POS_CURRENT || monitor != grid::POS_CURRENT) {
            return grid::set_position(gridpos, monitor) ?
                EXIT_SUCCESS : EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    default:
        ERROR("%s: no command specified", argv[0]);
        syntax(argv[0]);
        return EXIT_FAILURE;
    }
}
