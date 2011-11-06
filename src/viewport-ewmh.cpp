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

#include "viewport.h"
#include "x11-util.h"
#include "config.h"

bool viewport::get_viewport_ewmh(Display* disp,
		Dimensions& viewport_out) {
	//get current workspace
	unsigned long cur_workspace;
	{
		unsigned long* cur_ptr;
		if (!(cur_ptr = (unsigned long *)x11_util::get_property(disp, DefaultRootWindow(disp),
								XA_CARDINAL, "_NET_CURRENT_DESKTOP", NULL))) {
			ERROR_DIR("unable to retrieve current desktop");
			return false;
		}
		cur_workspace = *cur_ptr;
		x11_util::free_property(cur_ptr);
	}

	unsigned long* area;
	size_t area_count = 0;//number of areas returned, one per workspace. each area contains 4 ulongs.
	if (!(area = (unsigned long*)x11_util::get_property(disp, DefaultRootWindow(disp),
							XA_CARDINAL, "_NET_WORKAREA", &area_count))) {
		ERROR_DIR("unable to retrieve spanning workarea");
		return false;
	}
	if (area_count == 0) {
		ERROR_DIR("unable to retrieve spanning workarea.");
		x11_util::free_property(area);
		return false;
	}
	if (cur_workspace >= (area_count * 4) || area_count % 4 != 0) {//nice to have
		ERROR("got invalid workarea count: %d (cur workspace: %d)",
				area_count, cur_workspace);
		x11_util::free_property(area);
		return false;
	}
	if (config::debug_enabled) {
		for (size_t i = 0; i < area_count/4; ++i) {
			if (i == cur_workspace) {
				DEBUG("active workspace %lu of %lu: %lux %luy %luw %luh",
						i+1, area_count/4,
						area[i*4], area[i*4+1], area[i*4+2], area[i*4+3]);
			} else {
				DEBUG("inactive workspace %lu of %lu: %lux %luy %luw %luh",
						i+1, area_count/4,
						area[i*4], area[i*4+1], area[i*4+2], area[i*4+3]);
			}
		}
	}

	//set current workspace as viewport
	viewport_out.x = area[cur_workspace*4];
	viewport_out.y = area[(cur_workspace*4)+1];
	viewport_out.width = area[(cur_workspace*4)+2];
	viewport_out.height = area[(cur_workspace*4)+3];
	x11_util::free_property(area);
	return true;
}
