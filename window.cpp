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

#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>

#define MAX_PROPERTY_VALUE_LEN 4096
#define SOURCE_INDICATION 2 //say that we're a pager or taskbar

namespace {
	unsigned char* get_property(Display *disp, Window win,
			Atom xa_prop_type, const char* prop_name, size_t* out_count) {
		Atom xa_prop_name = XInternAtom(disp, prop_name, false);
		if (xa_prop_name == None) {
			ERROR("Atom not found for %s", prop_name);
			return NULL;
		}
		Atom xa_ret_type;
		int ret_format;
		unsigned long ret_nitems, ret_bytes_after;
		unsigned char* ret_prop;

		/* MAX_PROPERTY_VALUE_LEN / 4 explanation (XGetWindowProperty manpage):
		 *
		 * long_length = Specifies the length in 32-bit multiples of the
		 *               data to be retrieved.
		 */
		if (XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, false,
						xa_prop_type, &xa_ret_type, &ret_format,
						&ret_nitems, &ret_bytes_after, &ret_prop) != Success) {
			ERROR("Cannot get %s property.", prop_name);
			return NULL;
		}

		if (xa_ret_type != xa_prop_type) {
			if (xa_ret_type == None) {
				// avoid crash on XGetAtomName(None)
				char *req = XGetAtomName(disp, xa_prop_type);
				//not necessarily an error, can happen if the window in question just lacks the requested property
				//DEBUG("Unsupported or invalid request %s: requested type %s, got <none>", prop_name, req);
				XFree(req);
			} else {
				char *req = XGetAtomName(disp, xa_prop_type),
					*got = XGetAtomName(disp, xa_ret_type);
				ERROR("Invalid type of %s property: req %s, got %s",
						prop_name, req, got);
				XFree(req);
				XFree(got);
			}
			XFree(ret_prop);
			return NULL;
		}

		if (out_count != NULL) {
			*out_count = ret_nitems;
		}
		return ret_prop;
	}
	void free_property(void* prop) {
		XFree(prop);
	}

	int client_msg(Display* disp, Window win, const char *msg,
			unsigned long data0, unsigned long data1,
			unsigned long data2, unsigned long data3,
			unsigned long data4) {
		XEvent event;
		long mask = SubstructureRedirectMask | SubstructureNotifyMask;

		event.xclient.type = ClientMessage;
		event.xclient.serial = 0;
		event.xclient.send_event = True;
		event.xclient.message_type = XInternAtom(disp, msg, False);
		event.xclient.window = win;
		event.xclient.format = 32;
		event.xclient.data.l[0] = data0;
		event.xclient.data.l[1] = data1;
		event.xclient.data.l[2] = data2;
		event.xclient.data.l[3] = data3;
		event.xclient.data.l[4] = data4;

		DEBUG("send message_type=%lu, data=(%lu,%lu,%lu,%lu,%lu)",
				event.xclient.message_type, data0, data1, data2, data3, data4);

		if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
			return true;
		} else {
			ERROR("Cannot send %s event.", msg);
			return false;
		}
	}

	bool get_window_size(Display* disp, Window win,
			ActiveWindow::Dimensions* out_exterior = NULL,
			unsigned int* out_margin_width = NULL,
			unsigned int* out_margin_height = NULL) {
		unsigned int margin_width, margin_height;
		{
			size_t count = 0;
			unsigned int* widths;
			if (!(widths = (unsigned int*)get_property(disp, win,
									XA_CARDINAL, "_NET_FRAME_EXTENTS", &count))) {
				//apparently fails with eg chrome, so just assume 0 and move on
				//(yet chrome oddly works fine with unmaximize_unshade_window())
				DEBUG_DIR("get frame extents failed, assuming extents = 0");
				margin_width = margin_height = 0;
			} else {
				if (count != 4) {
					ERROR("got size %lu, want %lu", count, 4);
					free_property(widths);
					return false;
				}
				margin_width = widths[0] + widths[1];//left, right
				margin_height = widths[2] + widths[3];//top, bottom
				DEBUG("extents: width%u height%u",
						margin_width, margin_height);
				free_property(widths);
			}
		}

		//_NET_EXTENTS doesnt include window decoration (titlebar etc), get that here:

		long exterior_x, exterior_y;
		unsigned int interior_width, interior_height;
		Window rootwin;
		{
			unsigned int color_depth, border;
			int margin_left_tmp, margin_top_tmp;
			if (XGetGeometry(disp, win, &rootwin, &margin_left_tmp, &margin_top_tmp,
							&interior_width, &interior_height,
							&border, &color_depth) == 0) {
				ERROR_DIR("get geometry failed");
				return false;
			}

			//also get window coords (given as internal, subtract XGetGeo to get external)

			int interior_x, interior_y;
			if (XTranslateCoordinates(disp, win, rootwin, 0, 0,
							&interior_x, &interior_y, &rootwin) == 0) {
				ERROR_DIR("coordinate transform failed");
				return false;
			}

			//only use XGetGeo margins when calculating exterior position:
			exterior_x = interior_x - margin_left_tmp - border;
			exterior_y = interior_y - margin_top_tmp - border;
			DEBUG("pos: interior %ldx %ldy - geomargins %dw %dh %ub = exterior %ldx %ldy",
					interior_x, interior_y,
					margin_left_tmp, margin_top_tmp, border,
					exterior_x, exterior_y);

			//but use BOTH XGetGeo AND _NET_EXTENTS in calculating overall margin:
			margin_width += margin_left_tmp + border + border;
			margin_height += margin_top_tmp + border + border;
		}

		if (out_exterior != NULL) {
			out_exterior->x = exterior_x;
			out_exterior->y = exterior_y;
			out_exterior->width = interior_width + margin_width;
			out_exterior->height = interior_height + margin_height;
		}

		if (out_margin_width != NULL) {
			*out_margin_width = margin_width;
		}
		if (out_margin_height != NULL) {
			*out_margin_height = margin_height;
		}

		DEBUG("size: interior %luw %luh + summargins %dw %dh = %uw %uh",
				interior_width, interior_height, margin_width, margin_height,
				interior_width + margin_width, interior_height + margin_height);

		return true;
	}

	bool defullscreen_deshade_window(Display* disp, Window win) {
		//this disagrees with docs, which say that we should be using a
		//_NET_WM_STATE_DISABLE atom in data[0]. but that apparently doesn't
		//work in practice, but '0' does. (and '1' works for ENABLE)

		if (!client_msg(disp, win, "_NET_WM_STATE",
						0,//1 = enable state(s), 0 = disable state(s)
						XInternAtom(disp, "_NET_WM_STATE_SHADED", False),
						XInternAtom(disp, "_NET_WM_STATE_FULLSCREEN", False),
						SOURCE_INDICATION, 0)) {
			ERROR_DIR("couldnt unshade/defullscreen");
			return false;
		}
		return true;
	}

	bool demaximize_window(Display* disp, Window win) {
		//this disagrees with docs, which say that we should be using a
		//_NET_WM_STATE_DISABLE atom in data[0]. but that apparently doesn't
		//work in practice, but '0' does. (and '1' works for ENABLE)

		if (!client_msg(disp, win, "_NET_WM_STATE",
						0,//1 = enable state(s), 0 = disable state(s)
						XInternAtom(disp, "_NET_WM_STATE_MAXIMIZED_VERT", False),
						XInternAtom(disp, "_NET_WM_STATE_MAXIMIZED_HORZ", False),
						SOURCE_INDICATION, 0)) {
			ERROR_DIR("couldnt demaximize");
			return false;
		}

		return true;
	}

	bool _get_viewport_ewmh_workarea(Display* disp, ActiveWindow::Dimensions& viewport_out) {
		//get current workspace
		unsigned long cur_workspace;
		{
			unsigned long* cur_ptr;
			if (!(cur_ptr = (unsigned long *)get_property(disp, DefaultRootWindow(disp),
									XA_CARDINAL, "_NET_CURRENT_DESKTOP", NULL))) {
				ERROR_DIR("unable to retrieve current desktop");
				return false;
			}
			cur_workspace = *cur_ptr;
			free_property(cur_ptr);
		}

		unsigned long* area;
		size_t area_count = 0;//number of areas returned, one per workspace. each area contains 4 ulongs.
		if (!(area = (unsigned long*)get_property(disp, DefaultRootWindow(disp),
								XA_CARDINAL, "_NET_WORKAREA", &area_count))) {
			ERROR_DIR("unable to retrieve spanning workarea");
			return false;
		}
		if (area_count == 0) {
			ERROR_DIR("unable to retrieve spanning workarea.");
			free_property(area);
			return false;
		}
		if (cur_workspace >= (area_count * 4) || area_count % 4 != 0) {//nice to have
			ERROR("got invalid workarea count: %d (cur workspace: %d)",
					area_count, cur_workspace);
			free_property(area);
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
		free_property(area);
		return true;
	}

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
	int INTERSECTION(int a1, int a2, int b1, int b2) {
		int ret = (a2 < b1 || b2 < a1) ? 0 :
			(a1 >= b1) ?
				((a2 >= b2) ? (b2 - a1) : (a2 - a1)) :
				((a2 >= b2) ? (b2 - b1) : (a2 - b1));
		DEBUG("%d-%d x %d-%d = %d", a1, a2, b1, b2, ret);
		return ret;
	}

	bool _get_viewport_xinerama(Display* disp, const ActiveWindow::Dimensions& activewin,
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
		if (!(clients = (Window*)get_property(disp, DefaultRootWindow(disp),
								XA_WINDOW, "_NET_CLIENT_LIST", &client_count))) {
			ERROR_DIR("unable to retrieve list of clients");
			return false;
		}
		for (size_t i = 0; i < client_count; ++i) {
			unsigned long* strut;
			size_t strut_count = 0;//number of strut values for this client (should always be 12)
			if (!(strut = (unsigned long*)get_property(disp, clients[i],
									XA_CARDINAL, "_NET_WM_STRUT_PARTIAL", &strut_count))) {
				//DEBUG("client %lu of %lu lacks struts", i+1, client_count);
				continue;
			}
			if (strut_count != 12) {//nice to have
				ERROR_DIR("incorrect number of strut values: got %lu, expected 12", strut_count);
				free_property(strut);
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
			free_property(strut);
		}
		free_property(clients);
		return true;
	}
	bool get_viewport(Display* disp, const ActiveWindow::Dimensions& activewin,
			ActiveWindow::Dimensions& viewport_out) {
		//try xinerama, fall back to ewmh if xinerama is unavailable
		return _get_viewport_xinerama(disp, activewin, viewport_out) ||
			_get_viewport_ewmh_workarea(disp, viewport_out);
	}
}

ActiveWindow::ActiveWindow() {
	if (!(disp = XOpenDisplay(NULL))) {
		ERROR_DIR("unable to get display");
		return;
	}

	if (!(win = (Window*)get_property(disp, DefaultRootWindow(disp),
							XA_WINDOW, "_NET_ACTIVE_WINDOW", NULL))) {
		ERROR_DIR("unable to get active window");
	}
}

ActiveWindow::~ActiveWindow() {
	if (disp != NULL) {
		XCloseDisplay(disp);
	}
	if (win != NULL) {
		free_property(win);
	}
}

#define CHECK_STATE() if (disp == NULL || win == NULL) { \
		ERROR_DIR("unable to initialize active window"); \
		return false; \
	}

bool ActiveWindow::Sizes(Dimensions& viewport, Dimensions& activewin) const {
	CHECK_STATE();

	if (config::debug_enabled) {
		size_t count = 0;
		Atom* states;
		if (!(states = (Atom*)get_property(disp, *win,
								XA_ATOM, "_NET_WM_STATE", &count))) {
			ERROR_DIR("couldnt get window states");
		} else {
			for (size_t i = 0; i < count; ++i) {
				LOG("state %lu: %d %s",
						i, states[i], XGetAtomName(disp, states[i]));
			}
			free_property(states);
		}
	}

	if (!get_window_size(disp, *win, &activewin, NULL, NULL)) {
		ERROR_DIR("couldnt get window size");
		return false;
	}

	//special case: if the active window is fullscreen, the desktop workarea is
	//wrong (ie the obscured taskbar extents aren't included). get around this
	//by unfullscreening the window first (if applicable). but return the
	//window's dimensions from when it was fullscreened/shaded.
	defullscreen_deshade_window(disp, *win);//disregard failure

	if (!get_viewport(disp, activewin, viewport)) {
		ERROR_DIR("unable to get viewport dimensions");
		return false;
	}

	DEBUG("viewport %dx %dy %luw %luh, activewin %dx %dy %luw %luh",
			viewport.x, viewport.y, viewport.width, viewport.height,
			activewin.x, activewin.y, activewin.width, activewin.height);

	return true;
}

bool ActiveWindow::MoveResize(const Dimensions& activewin) {
	CHECK_STATE();

	unsigned int margin_width, margin_height;
	if (!get_window_size(disp, *win, NULL, &margin_width, &margin_height)) {
		return false;
	}

	//demaximize the window before attempting to move it
	demaximize_window(disp, *win);//disregard failure

	unsigned long new_interior_width = activewin.width - margin_width,
		new_interior_height = activewin.height - margin_height;

	//moveresize uses exterior for position, but interior for width/height
	DEBUG("%ldx %ldy %luw %luh - margins %dw %dh = %ldx %ldy %luw %luh",
			activewin.x, activewin.y, activewin.width, activewin.height,
			margin_width, margin_height,
			activewin.x, activewin.y, new_interior_width, new_interior_height);

	return XMoveResizeWindow(disp, *win, activewin.x, activewin.y,
			new_interior_width, new_interior_height) == 0;
}
