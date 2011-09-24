#ifndef GRIDMGR_VIEWPORT_H
#define GRIDMGR_VIEWPORT_H

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

#include "window.h"
#include "config.h"

namespace viewport {

	bool get_viewport_ewmh(Display* disp,
			ActiveWindow::Dimensions& viewport_out);

#ifdef USE_XINERAMA
	bool get_viewport_xinerama(Display* disp,
			const ActiveWindow::Dimensions& activewin,
			ActiveWindow::Dimensions& viewport_out);
#endif

}

#endif
