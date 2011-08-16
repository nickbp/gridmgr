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

#include <X11/extensions/Xinerama.h>

#ifndef USE_XINERAMA
#error "Build configuration error:"
#error " Shouldn't be building this file if USE_XINERAMA is disabled."
#endif

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
static int INTERSECTION(int a1, int a2, int b1, int b2) {
	int ret = (a2 < b1 || b2 < a1) ? 0 :
		(a1 >= b1) ?
		((a2 >= b2) ? (b2 - a1) : (a2 - a1)) :
		((a2 >= b2) ? (b2 - b1) : (a2 - b1));
	DEBUG("%d-%d x %d-%d = %d", a1, a2, b1, b2, ret);
	return ret;
}

bool viewport::get_viewport_xinerama(Display* disp,
		const ActiveWindow::Dimensions& activewin,
		ActiveWindow::Dimensions& viewport_out) {
	//pick the xinerama screen which the active window 'belongs' to (= 'active screen')
	//also calculate a bounding box across all screens (needed for strut math)
	long bound_x1 = 0, bound_x2 = 0, bound_y1 = 0, bound_y2 = 0,//bounding box of all screens
		active_x1 = 0, active_x2 = 0, active_y1 = 0, active_y2 = 0;//copy of the active screen
	{
		int screen_count = 0;
		XineramaScreenInfo* screens = XineramaQueryScreens(disp, &screen_count);
		if (screens == NULL || screen_count == 0) {
			DEBUG_DIR("xinerama disabled");
			if (screens != NULL) {
				XFree(screens);
			}
			return false;
		} else {
			//initialize bounding box to something
			bound_x1 = screens[0].x_org;
			bound_x2 = screens[0].x_org + screens[0].width;
			bound_y1 = screens[0].y_org;
			bound_y2 = screens[0].y_org + screens[0].height;

			//search for largest overlap between active window and xinerama screen.
			//the screen with the most overlap is the 'active screen'
			int active_i = 0, active_overlap = 0;

			for (int i = 0; i < screen_count; ++i) {
				XineramaScreenInfo& screen = screens[i];

				//grow bounding box
				bound_x1 = MIN(bound_x1, screen.x_org);
				bound_x2 = MAX(bound_x2, screen.x_org + screen.width);
				bound_y1 = MIN(bound_y1, screen.y_org);
				bound_y2 = MAX(bound_y2, screen.y_org + screen.height);

				//check overlap, update counters if this overlap is bigger
				int overlap =
					INTERSECTION(screen.x_org, screen.x_org+screen.width,
							activewin.x, activewin.x+activewin.width) *
					INTERSECTION(screen.y_org, screen.y_org+screen.height,
							activewin.y, activewin.y+activewin.height);

				DEBUG("screen %d of %d (overlap %d): %dx %dy %dw %dh",
						i+1, screen_count, overlap,
						screens[i].x_org, screens[i].y_org,
						screens[i].width, screens[i].height);

				if (active_overlap < overlap) {
					active_overlap = overlap;
					active_i = i;
				}
			}

			viewport_out.x = screens[active_i].x_org;
			viewport_out.y = screens[active_i].y_org;
			viewport_out.width = screens[active_i].width;
			viewport_out.height = screens[active_i].height;
			active_x1 = viewport_out.x;
			active_x2 = viewport_out.x + viewport_out.width;
			active_y1 = viewport_out.y;
			active_y2 = viewport_out.y + viewport_out.height;
		}
		XFree(screens);
	}
	DEBUG("screen bounding box: %ld-%ldx %ld-%ldy",
			bound_x1, bound_x2, bound_y1, bound_y2);

	//now that we've got the active screen and the bounding box,
	//iterate over all struts, shrinking the active screen's viewport as necessary
	size_t client_count = 0;
	Window* clients;
	if (!(clients = (Window*)x11_util::get_property(disp, DefaultRootWindow(disp),
							XA_WINDOW, "_NET_CLIENT_LIST", &client_count))) {
		ERROR_DIR("unable to retrieve list of clients");
		return false;
	}
	for (size_t i = 0; i < client_count; ++i) {
		unsigned long* strut;
		size_t strut_count = 0;//number of strut values for this client (should always be 12)
		if (!(strut = (unsigned long*)x11_util::get_property(disp, clients[i],
								XA_CARDINAL, "_NET_WM_STRUT_PARTIAL", &strut_count))) {
			//DEBUG("client %lu of %lu lacks struts", i+1, client_count);
			continue;
		}
		if (strut_count != 12) {//nice to have
			ERROR_DIR("incorrect number of strut values: got %lu, expected 12", strut_count);
			x11_util::free_property(strut);
			return false;
		}
		DEBUG("client %lu of %lu struts: left:%lux%lu-%lu right:%lux%lu-%lu top:%lux%lu-%lu bot:%lux%lu-%lu",
				i+1, client_count,
				strut[0], strut[4], strut[5],
				strut[1], strut[6], strut[7],
				strut[2], strut[8], strut[9],
				strut[3], strut[10], strut[11]);
		//check whether the strut is within the active screen,
		//then update/shrink the active screen's viewport if it is.
		if (strut[0] > 0 && INTERSECTION(active_y1, active_y2, strut[4], strut[5]) != 0) {//left
			viewport_out.x = MAX(viewport_out.x, strut[0] - bound_x1);
		}
		if (strut[1] > 0 && INTERSECTION(active_y1, active_y2, strut[6], strut[7]) != 0) {//right
			viewport_out.width = MIN(viewport_out.width, bound_x2 - strut[1] - viewport_out.x);
		}
		if (strut[2] > 0 && INTERSECTION(active_x1, active_x2, strut[8], strut[9]) != 0) {//top
			viewport_out.y = MAX(viewport_out.y, strut[2] - bound_y1);
		}
		if (strut[3] > 0 && INTERSECTION(active_x1, active_x2, strut[10], strut[11]) != 0) {//bot
			viewport_out.height = MIN(viewport_out.height, bound_y2 - strut[3] - viewport_out.y);
		}
		x11_util::free_property(strut);
	}
	x11_util::free_property(clients);
	return true;
}
