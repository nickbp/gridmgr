#ifndef GRIDMGR_POS_H
#define GRIDMGR_POS_H

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

namespace grid {
	/* These are the various available positions which may be set.
	 * CURRENT is "use the current position", useful when only switching monitors */
	enum POS {
		POS_UNKNOWN, POS_CURRENT,
		POS_TOP_LEFT, POS_TOP_CENTER, POS_TOP_RIGHT,
		POS_LEFT, POS_CENTER, POS_RIGHT,
		POS_BOT_LEFT, POS_BOT_CENTER, POS_BOT_RIGHT
	};

	inline const char* pos_str(POS pos) {
		switch (pos) {
		case POS_UNKNOWN:
			return "UNKNOWN";
		case POS_CURRENT:
			return "CURRENT";
		case POS_TOP_LEFT:
			return "TOP_LEFT";
		case POS_TOP_CENTER:
			return "TOP_CENTER";
		case POS_TOP_RIGHT:
			return "TOP_RIGHT";
		case POS_LEFT:
			return "LEFT";
		case POS_CENTER:
			return "CENTER";
		case POS_RIGHT:
			return "RIGHT";
		case POS_BOT_LEFT:
			return "BOT_LEFT";
		case POS_BOT_CENTER:
			return "BOT_CENTER";
		case POS_BOT_RIGHT:
			return "BOT_RIGHT";
		}
		return "???";
	}

	/* These are the available directions for movement between monitors and/or
	 * windows. CURRENT is "use the current monitor/window". */
	enum DIR {
		DIR_UNKNOWN, DIR_CURRENT,
		DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT
	};

	inline const char* dir_str(DIR dir) {
		switch (dir) {
		case DIR_UNKNOWN:
			return "UNKNOWN";
		case DIR_CURRENT:
			return "CURRENT";
		case DIR_UP:
			return "UP";
		case DIR_RIGHT:
			return "RIGHT";
		case DIR_DOWN:
			return "DOWN";
		case DIR_LEFT:
			return "LEFT";
		}
		return "???";
	}
}

#endif
