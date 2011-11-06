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

#include "grid.h"

#include "position.h"
#include "window.h"

bool grid::set_position(POS pos) {
	//initializes to the currently active window
	ActiveWindow win;

	//get current window's dimensions
	Dimensions viewport, cur_dim;
	if (!win.Sizes(viewport, cur_dim)) {
		return false;
	}

	//using the requested position and current dimensions,
	//calculate and set new dimensions
	PositionCalc calc(viewport, cur_dim);
	
	Dimensions next_dim;
	return calc.NextPos(pos, next_dim) && win.MoveResize(next_dim);
}
