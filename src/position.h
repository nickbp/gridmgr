#ifndef GRIDMGR_POSITION_H
#define GRIDMGR_POSITION_H

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
	State() : pos(grid::POS_UNKNOWN), mode(grid::MODE_UNKNOWN) { }

	grid::POS pos;
	grid::MODE mode;
};

class PositionCalc {
public:
	PositionCalc(const Dimensions& window)
		: window(window) { }

	/* Produces an autodetected state of this window using its current
	 * coordinates. Returns true on success, else false. */
	bool CurState(const Dimensions& viewport, State& cur_state) const;

	/* Given a current state and requested position for the window, calculates
	 * its next state. Returns true on success, else false. */
	bool NextState(const State& cur_state, grid::POS req_pos,
			State& next_state) const;

	/* Given a state for the window, calculates the dimensions of that position.
	 * Returns true on success, else false. */
	bool StateToDim(const Dimensions& viewport, const State& state,
			Dimensions& out) const;

	/* Special case of StateToDim: Given the current viewport and next viewport
	 * for a window with no autodetected state, calculates a reasonable location
	 * in the new viewport. */
	void ViewportToDim(const Dimensions& cur_viewport,
			const Dimensions& next_viewport, Dimensions& out) const;

private:
	const Dimensions window;
};

#endif
