#ifndef GRIDMGR_NEIGHBOR_H
#define GRIDMGR_NEIGHBOR_H

/*
  gridmgr - Organizes windows according to a grid.
  Copyright (C) 2012  Nicholas Parker

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

#include <vector>

#include "dimensions.h"
#include "pos.h"

typedef std::vector<Dimensions> dim_list_t;

namespace neighbor {
	void select(grid::POS dir, const dim_list_t& all, size_t active, size_t& select);
}

#endif
