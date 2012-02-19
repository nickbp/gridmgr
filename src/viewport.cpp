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

#include <cstddef>

#include "config.h"
#include "neighbor.h"
#include "viewport.h"

#include "viewport-imp-ewmh.h"
#ifdef USE_XINERAMA
#include "viewport-imp-xinerama.h"
#endif

namespace {
	bool get_all(const Dimensions& activewin,
			dim_list_t& viewports, size_t& active) {
		Display* disp = XOpenDisplay(NULL);
		if (disp == NULL) {
			ERROR_DIR("unable to get display");
			return false;
		}

#ifdef USE_XINERAMA
		//try xinerama, fall back to ewmh if xinerama is unavailable
		bool ok = viewport::xinerama::get_viewports(disp, activewin, viewports, active) ||
			viewport::ewmh::get_viewports(disp, activewin, viewports, active);
#else
		//xinerama disabled; just do ewmh
		bool ok = viewport::ewmh::get_viewports(disp, activewin, viewports, active);
#endif

		if (config::debug_enabled) {
			for (size_t i = 0; i < viewports.size(); ++i) {
				const Dimensions& v = viewports[i];
				DEBUG("viewport %lu: %dx %dy %luw %luh",
						i, v.x, v.y, v.width, v.height);
			}
		}

		XCloseDisplay(disp);
		return ok;
	}
}

bool ViewportCalc::Viewports(grid::POS monitor,
		Dimensions& cur_viewport, Dimensions& next_viewport) const {
	dim_list_t viewports;
	size_t active, neighbor;
	if (!get_all(activewin, viewports, active)) {
		return false;
	}

	neighbor::select(monitor, viewports, active, neighbor);

	cur_viewport = viewports[active];
	next_viewport = viewports[neighbor];

	DEBUG("cur viewport: %dx %dy %dw %dh",
			cur_viewport.x, cur_viewport.y, cur_viewport.width, cur_viewport.height);
	DEBUG("next viewport: %dx %dy %dw %dh",
			next_viewport.x, next_viewport.y, next_viewport.width, next_viewport.height);
	return true;
}
