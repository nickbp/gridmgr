#ifndef GRIDMGR_WINDOW_H
#define GRIDMGR_WINDOW_H

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

#include <X11/Xlib.h>

/*
  NOTE: This class internally assumes that the window/viewport aren't altered
  throughout the lifetime of the constructed object. Any alterations
  will NOT be automatically detected!
*/

class ActiveWindow {
 public:
	ActiveWindow();
	virtual ~ActiveWindow();

	struct Size {
		unsigned long width;
		unsigned long height;
	};
	struct Dimensions {
		//x,y are relative to the viewport
		long x;
		long y;
		//width,height include any borders/decorations
		unsigned long width;
		unsigned long height;
	};

	bool Sizes(Size& viewport, Dimensions& activewin) const;
	bool MoveResize(const Dimensions& activewin);

 private:
	Display* disp;
	Window* win;
	Dimensions _viewport;
};

#endif
