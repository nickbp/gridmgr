#ifndef GRIDMGR_WINDOW_H
#define GRIDMGR_WINDOW_H

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

#include <X11/Xlib.h>

#include "pos.h"
#include "dimensions.h"

namespace window {
    /* Finds the nearest window in the given direction and activates it. */
    bool select_activate(grid::POS dir);
}

class ActiveWindow {
public:
    ActiveWindow() : disp(NULL), win(NULL) { }
    virtual ~ActiveWindow();

    bool Size(Dimensions& activewin);

    bool MoveResize(const Dimensions& activewin);

    bool Maximize();
    bool DeFullscreen();
    bool DeShade();

private:
    bool init();

    Display* disp;
    Window* win;
};

#endif
