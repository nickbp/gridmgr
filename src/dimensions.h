#ifndef GRIDMGR_DIMENSIONS_H
#define GRIDMGR_DIMENSIONS_H

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

struct Dimensions {
	//position of the top left corner of the screen
	long x;
	long y;
	//width,height are 'exterior' size, including any borders/decorations
	unsigned long width;
	unsigned long height;
};

#endif
