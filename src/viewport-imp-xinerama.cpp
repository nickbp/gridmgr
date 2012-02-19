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

#include <X11/extensions/Xinerama.h>

#include "config.h"
#include "viewport-imp-xinerama.h"
#include "x11-util.h"

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

namespace {
	bool get_screens(Display* disp, const Dimensions& activewin,
			Dimensions& bounding_box, dim_list_t& viewports,
			size_t& active_viewport) {
		int screen_count = 0;
		XineramaScreenInfo* screens = XineramaQueryScreens(disp, &screen_count);
		if (screens == NULL || screen_count == 0) {
			DEBUG_DIR("xinerama not loaded or unavailable");
			if (screens != NULL) {
				XFree(screens);
			}
			return false;
		}

		//initialize bounding box to something
		bounding_box.x = screens[0].x_org;
		bounding_box.y = screens[0].y_org;
		int bound_max_x = screens[0].x_org + screens[0].width,
			bound_max_y = screens[0].y_org + screens[0].height;

		// search for largest overlap between active window and xinerama screen.
		// the screen with the most overlap is the 'active screen'
		int active_overlap = 0;

		for (int i = 0; i < screen_count; ++i) {
			const XineramaScreenInfo& screen = screens[i];

			// grow bounding box
			bounding_box.x = MIN(bounding_box.x, screen.x_org);
			bounding_box.y = MIN(bounding_box.y, screen.y_org);
			bound_max_x = MAX(bound_max_x, screen.x_org + screen.width);
			bound_max_y = MAX(bound_max_y, screen.y_org + screen.height);

			// add viewport
			viewports.push_back(Dimensions());
			Dimensions& v = viewports.back();
			v.x = screen.x_org;
			v.y = screen.y_org;
			v.width = screen.width;
			v.height = screen.height;

			// check active overlap
			int overlap =
				INTERSECTION(screen.x_org, screen.x_org + screen.width,
						activewin.x, activewin.x + activewin.width) *
				INTERSECTION(screen.y_org, screen.y_org + screen.height,
						activewin.y, activewin.y + activewin.height);

			DEBUG("screen %d of %d: %dx %dy %dw %dh (overlap %d)",
					i+1, screen_count,
					screen.x_org, screen.y_org, screen.width, screen.height,
					overlap);

			if (overlap > active_overlap) {
				// overlap is bigger, update active counters
				active_overlap = overlap;
				active_viewport = i;
			}
		};

		DEBUG("active screen is %lu of %d", active_viewport+1, screen_count);

		bounding_box.width = bound_max_x - bounding_box.x;
		bounding_box.height = bound_max_y - bounding_box.y;
		DEBUG("desktop bounding box: %ldx %ldy %ldw %ldh",
				bounding_box.x, bounding_box.y,
				bounding_box.width, bounding_box.height);

		XFree(screens);
		return true;
	}

	struct strut {
		enum TYPE {
			LEFT, RIGHT, TOP, BOTTOM
		};

		strut(TYPE type, size_t width, size_t min, size_t max)
			: type(type), width(width), min(min), max(max) { }

		TYPE type;
		size_t width, min, max;
	};

	bool get_struts(Display* disp, std::vector<strut>& out) {
		Window* clients;
		size_t client_count = 0;
		static Atom clientlist_msg = XInternAtom(disp, "_NET_CLIENT_LIST", False);
		if (!(clients = (Window*)x11_util::get_property(disp, DefaultRootWindow(disp),
								XA_WINDOW, clientlist_msg, &client_count))) {
			ERROR_DIR("unable to retrieve list of clients");
			return false;
		}
		for (size_t i = 0; i < client_count; ++i) {
			unsigned long* xstrut;
			size_t xstrut_count = 0;//number of strut values for this client (should always be 12)
			static Atom strut_msg = XInternAtom(disp, "_NET_WM_STRUT_PARTIAL", False);
			if (!(xstrut = (unsigned long*)x11_util::get_property(disp, clients[i],
									XA_CARDINAL, strut_msg, &xstrut_count))) {
				//DEBUG("client %lu of %lu lacks struts", i+1, client_count);
				continue;
			}
			if (xstrut_count != 12) {//nice to have
				ERROR_DIR("incorrect number of strut values: got %lu, expected 12", xstrut_count);
				x11_util::free_property(clients);
				x11_util::free_property(xstrut);
				return false;
			}

			DEBUG("client %lu of %lu struts: left:%lu@%lu-%lu right:%lu@%lu-%lu top:%lu@%lu-%lu bot:%lu@%lu-%lu",
					i+1, client_count,
					xstrut[0], xstrut[4], xstrut[5],
					xstrut[1], xstrut[6], xstrut[7],
					xstrut[2], xstrut[8], xstrut[9],
					xstrut[3], xstrut[10], xstrut[11]);

			//left
			if (xstrut[0] > 0) {
				out.push_back(strut(strut::LEFT, xstrut[0], xstrut[4], xstrut[5]));
			}
			//right
			if (xstrut[1] > 0) {
				out.push_back(strut(strut::RIGHT, xstrut[1], xstrut[6], xstrut[7]));
			}
			//top
			if (xstrut[2] > 0) {
				out.push_back(strut(strut::TOP, xstrut[2], xstrut[8], xstrut[9]));
			}
			//bot
			if (xstrut[3] > 0) {
				out.push_back(strut(strut::BOTTOM, xstrut[3], xstrut[10], xstrut[11]));
			}

			x11_util::free_property(xstrut);
		}
		x11_util::free_property(clients);
		return true;
	}

	void trim_screen(const Dimensions& bound, const std::vector<strut>& struts,
			Dimensions& screen) {
		//for simpler math, operate on things in terms of min/max
		long screen_max_x = screen.x + screen.width,
			screen_max_y = screen.y + screen.height;

		for (std::vector<strut>::const_iterator iter = struts.begin();
			 iter != struts.end(); ++iter) {
			switch (iter->type) {
			case strut::LEFT:
				// first check if it intersects our screen's min/max y
				if (INTERSECTION(screen.y, screen_max_y, iter->min, iter->max) != 0) {
					//then check if the strut (relative to the bounding box) actually exceeds our min x
					screen.x = MAX(screen.x, (long)iter->width - bound.x);
				}
				break;
			case strut::RIGHT:
				if (INTERSECTION(screen.y, screen_max_y, iter->min, iter->max) != 0) {
					long bound_max_x = bound.x + bound.width;
					screen_max_x = MIN(screen_max_x, bound_max_x - iter->width);
				}
				break;
			case strut::TOP:
				if (INTERSECTION(screen.x, screen_max_x, iter->min, iter->max) != 0) {
					screen.y = MAX(screen.y, (long)iter->width - bound.y);
				}
				break;
			case strut::BOTTOM:
				if (INTERSECTION(screen.x, screen_max_x, iter->min, iter->max) != 0) {
					long bound_max_y = bound.y + bound.height;
					screen_max_y = MIN(screen_max_y, bound_max_y - iter->width);
				}
				break;
			}
		}

		screen.width = screen_max_x - screen.x;
		screen.height = screen_max_y - screen.y;

		DEBUG("trimmed: %ldx %ldy %ldw %ldh",
				screen.x, screen.y, screen.width, screen.height);
	}
}

bool viewport::xinerama::get_viewports(Display* disp, const Dimensions& activewin,
		dim_list_t& viewports_out, size_t& active_out) {
	viewports_out.clear();

	Dimensions bounding_box;
	if (!get_screens(disp, activewin, bounding_box, viewports_out, active_out)) {
		return false;
	}

	std::vector<strut> struts;
	if (!get_struts(disp, struts)) {
		return false;
	}

	// trim struts from viewports
	for (dim_list_t::iterator iter = viewports_out.begin();
		 iter != viewports_out.end(); ++iter) {
		trim_screen(bounding_box, struts, *iter);
	}

	return true;
}
