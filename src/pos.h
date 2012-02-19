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
		POS_UP_LEFT, POS_UP_CENTER, POS_UP_RIGHT,
		POS_LEFT, POS_CENTER, POS_RIGHT,
		POS_DOWN_LEFT, POS_DOWN_CENTER, POS_DOWN_RIGHT
	};

	inline const char* pos_str(POS pos) {
		switch (pos) {
		case POS_UNKNOWN:
			return "UNKNOWN";
		case POS_CURRENT:
			return "CURRENT";
		case POS_UP_LEFT:
			return "UP_LEFT";
		case POS_UP_CENTER:
			return "UP_CENTER";
		case POS_UP_RIGHT:
			return "UP_RIGHT";
		case POS_LEFT:
			return "LEFT";
		case POS_CENTER:
			return "CENTER";
		case POS_RIGHT:
			return "RIGHT";
		case POS_DOWN_LEFT:
			return "DOWN_LEFT";
		case POS_DOWN_CENTER:
			return "DOWN_CENTER";
		case POS_DOWN_RIGHT:
			return "DOWN_RIGHT";
		}
		return "???";
	}
}

#endif
