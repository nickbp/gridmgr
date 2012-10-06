#ifndef GRIDMGR_GRID_H
#define GRIDMGR_GRID_H

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

namespace grid {
    /* Selects and makes active the window in the specified direction relative
     * to the currently active window. */
    bool set_active(POS window);

    /* Selects the active window and moves/resizes it to the requested
     * position/monitor, according to its current state.
     * Returns true if successful, false otherwise. */
    bool set_position(POS gridpos, POS monitor);
}

#endif
