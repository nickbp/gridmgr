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

#include "config.h"
#include "grid.h"
#include "position.h"
#include "viewport.h"
#include "window.h"

bool grid::set_active(POS window) {
    return window::select_activate(window);
}

bool grid::set_position(POS gridpos, POS monitor) {
    // initializes to the currently active window
    ActiveWindow win;

    // get current window's dimensions
    Dimensions cur_window;
    if (!win.Size(cur_window)) {
        return false;
    }

    /* special case: if the active window is fullscreen, the desktop workarea is
       wrong (ie the obscured taskbar extents aren't included). get around this
       by unfullscreening the window first (if applicable). but return the
       window's dimensions from when it was fullscreened/shaded. */
    win.DeFullscreen();// disregard failure

    Dimensions cur_viewport, next_viewport;
    {
        ViewportCalc vcalc(cur_window);
        // cur_window + monitor -> cur_viewport + next_viewport
        if (!vcalc.Viewports(monitor, cur_viewport, next_viewport)) {
            return false;
        }
    }

    /* using the requested position and current dimensions,
       calculate and set new dimensions */
    PositionCalc pcalc(cur_window);

    State cur_state, next_state;
    if (/* cur_viewport + cur_window -> cur_state */
            !pcalc.CurState(cur_viewport, cur_state) ||
            /* cur_state + pos -> next_state */
            !pcalc.NextState(cur_state, gridpos, next_state)) {
        return false;
    }

    Dimensions next_dim;
    if (next_state.pos == POS_UNKNOWN && next_state.mode == MODE_UNKNOWN) {
        // window doesnt currently have a state, and user didn't specify one
        pcalc.ViewportToDim(cur_viewport, next_viewport, next_dim);
    } else {
        // next_viewport + next_state -> next_dim
        if (!pcalc.StateToDim(next_viewport, next_state, next_dim)) {
            return false;
        }
    }

    // move the window to next_dim
    if (!win.DeShade() || !win.MoveResize(next_dim)) {
        return false;
    }

    if (next_state.pos == POS_CENTER &&
        next_state.mode == MODE_THREE_COL_L) {
        /* if we're going to be filling the screen anyway, just maximize
           (do this after MoveResize so that viewport changes are respected) */
        win.Maximize();// disregard failure
    }
    return true;
}
