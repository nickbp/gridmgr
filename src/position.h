#ifndef GRIDMGR_POSITION_H
#define GRIDMGR_POSITION_H

/*
  gridmgr - Organizes windows according to a grid.
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

#include "pos.h"
#include "dimensions.h"

namespace grid {
	enum MODE {
		MODE_UNKNOWN,
		MODE_TWO_COL,// 2x2 (not applicable for center col positions)
		MODE_THREE_COL_S,// 3x2 small: each position filling 1 column
		MODE_THREE_COL_L// 3x2 large: sides filling 2 columns and center filling full width
	};
}

struct State {
	grid::POS pos;
	grid::MODE mode;
};

class PositionCalc {
 public:
	PositionCalc(const Dimensions& viewport,
		 const Dimensions& window)
	 : viewport(viewport), window(window) { }

	/* Produces an autodetected state of this window using its current
	 * coordinates. Returns true on success, else false. */
	bool CurState(State& cur_state);

	/* Given a current state and requested position for the window, calculates
	 * its next state. Returns true on success, else false. */
	bool NextState(const State& cur_state, grid::POS req_pos,
			State& next_state);

	/* Given a state for the window, calculates the dimensions of that position.
	 * Returns true on success, else false. */
	bool StateToDim(const State& state, Dimensions& out);

 private:
	const Dimensions viewport, window;
};

#endif
