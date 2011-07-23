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

#define MAX_PROPERTY_VALUE_LEN 4096
#define SOURCE_INDICATION 2 //say that we're a pager or taskbar

namespace {
	unsigned char* get_property(Display *disp, Window win,
			Atom xa_prop_type, const char* prop_name, size_t* out_count) {
		Atom xa_prop_name = XInternAtom(disp, prop_name, false);
		if (xa_prop_name == None) {
			config::error("Atom not found for %s", prop_name);
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
			config::error("Cannot get %s property.", prop_name);
			return NULL;
		}

		if (xa_ret_type != xa_prop_type) {
			if (xa_ret_type == None) {
				// avoid crash on XGetAtomName(None)
				config::error("Invalid type of %s property: req %d, got %d",
						prop_name, xa_prop_type, xa_ret_type);
			} else {
				char *req = XGetAtomName(disp, xa_prop_type),
					*got = XGetAtomName(disp, xa_ret_type);
				config::error("Invalid type of %s property: req %s, got %s",
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

		config::debug("send message_type=%lu, data=(%lu,%lu,%lu,%lu,%lu)",
				event.xclient.message_type, data0, data1, data2, data3, data4);

		if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
			return true;
		} else {
			config::error("Cannot send %s event.", msg);
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
				config::debug("get frame extents failed, assuming extents = 0");
				margin_width = margin_height = 0;
			} else {
				if (count != 4) {
					config::error("get_window_size: got size %lu want %lu", count, 4);
					free_property(widths);
					return false;
				}
				margin_width = widths[0] + widths[1];//left, right
				margin_height = widths[2] + widths[3];//top, bottom
				config::debug("extents: width%u height%u",
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
				config::error("get_window_size: get geometry failed");
				return false;
			}

			//also get window coords (given as internal, subtract XGetGeo to get external)

			int interior_x, interior_y;
			if (XTranslateCoordinates(disp, win, rootwin, 0, 0,
							&interior_x, &interior_y, &rootwin) == 0) {
				config::error("get_window_size: coordinate transform failed");
				return false;
			}

			//only use XGetGeo margins when calculating exterior position:
			exterior_x = interior_x - margin_left_tmp - border;
			exterior_y = interior_y - margin_top_tmp - border;
			config::debug("pos: interior %ldx %ldy - geomargins %dw %dh %ldb = exterior %ldx %ldy",
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

		config::debug("size: interior %luw %luh + summargins %dw %dh = %uw %uh",
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
			config::error("couldnt unshade/defullscreen");
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
			config::error("couldnt demaximize");
			return false;
		}

		return true;
	}

	bool get_viewport(Display* disp, ActiveWindow::Dimensions& viewport) {
		unsigned long cur_desktop;
		{
			unsigned long* cur_ptr;
			if (!(cur_ptr = (unsigned long *)get_property(disp, DefaultRootWindow(disp),
									XA_CARDINAL, "_NET_CURRENT_DESKTOP", NULL)) &&
					!(cur_ptr = (unsigned long *)get_property(disp, DefaultRootWindow(disp),
									XA_CARDINAL, "_WIN_WORKSPACE", NULL))) {
				return false;
			}
			cur_desktop = *cur_ptr;
			free_property(cur_ptr);
		}

		size_t count = 0;
		unsigned long* area;
		if (!(area = (unsigned long*)get_property(disp, DefaultRootWindow(disp),
								XA_CARDINAL, "_NET_WORKAREA", &count)) &&
				!(area = (unsigned long*)get_property(disp, DefaultRootWindow(disp),
								XA_CARDINAL, "_WIN_WORKAREA", &count))) {
			return false;
		}
		if (count == 0) {
			config::error("Unable to retrieve viewport area.");
			free_property(area);
			return false;
		}

		//select cur desktop if available, fall back to 0 if not
		unsigned long cur_i = 0;
		if (cur_desktop < count) {
			cur_i = cur_desktop;
		}
		viewport.x = area[cur_i*4];
		viewport.y = area[(cur_i*4)+1];
		viewport.width = area[(cur_i*4)+2];
		viewport.height = area[(cur_i*4)+3];
		if (config::debug_enabled) {
			for (size_t i = 0; i < count/4; ++i) {
				if (i == cur_desktop) {
					config::debug("active desktop %lu of %lu: %lux %luy %luw %luh",
							i+1, count/4,
							area[i*4], area[i*4+1], area[i*4+2], area[i*4+3]);
				} else {
					config::debug("inactive desktop %lu of %lu: %lux %luy %luw %luh",
							i+1, count/4,
							area[i*4], area[i*4+1], area[i*4+2], area[i*4+3]);
				}
			}
		}

		free_property(area);
		return true;
	}
}

ActiveWindow::ActiveWindow() {
	disp = XOpenDisplay(NULL);
	if (disp == NULL) {
		config::error("unable to get display");
		return;
	}

	if (!get_viewport(disp, _viewport)) {
		config::error("unable to get viewport dimensions");
		return;
	}

	win = (Window*)get_property(disp, DefaultRootWindow(disp),
			XA_WINDOW, "_NET_ACTIVE_WINDOW", NULL);
	if (win == NULL) {
		config::error("unable to get active window");
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

#define CHECK_STATE() if (disp == NULL || win == NULL) {			\
		config::error("Unable to initialize active window");		\
		return false;												\
	}

bool ActiveWindow::Sizes(Size& viewport, Dimensions& activewin) const {
	CHECK_STATE();

	//first get the window dimensions, after unshading/fullscreening it

	if (config::debug_enabled) {
		size_t count = 0;
		Atom* states = (Atom*)get_property(disp, *win,
				XA_ATOM, "_NET_WM_STATE", &count);
		if (states == NULL) {
			config::error("couldnt get states");
		} else {
			for (size_t i = 0; i < count; ++i) {
				config::log("state %lu: %d %s",
						i, states[i], XGetAtomName(disp, states[i]));
			}
			free_property(states);
		}
	}

	if (!get_window_size(disp, *win, &activewin, NULL, NULL)) {
		config::error("couldnt get window size");
		return false;
	}

	//now get the desktop dimensions

	//special case: if the active window is fullscreen, the desktop workarea is
	//wrong (ie the obscured taskbar extents aren't included). get around this
	//by unfullscreening the window first (if applicable). but return the
	//window's dimensions from when it was fullscreened/shaded.
	defullscreen_deshade_window(disp, *win);//disregard failure

	//subtract viewport x,y from activewin so activewin is relative to viewport

	config::debug("sizes: viewport %dx %dy %luw %luh, activewin %dx %dy %luw %luh = activewin_adj %dx %dy",
			_viewport.x, _viewport.y, _viewport.width, _viewport.height,
			activewin.x, activewin.y, activewin.width, activewin.height,
			activewin.x - _viewport.x, activewin.y - _viewport.y);

	viewport.width = _viewport.width;
	viewport.height = _viewport.height;
	activewin.x -= _viewport.x;
	activewin.y -= _viewport.y;

	return true;
}

bool ActiveWindow::MoveResize(const Dimensions& activewin) {
	CHECK_STATE();

	unsigned int margin_width, margin_height;
	if (!get_window_size(disp, *win, NULL, &margin_width, &margin_height)) {
		return false;
	}

	//demaximize the window before moving it (if applicable)
	demaximize_window(disp, *win);//disregard failure

	unsigned long new_interior_width = activewin.width - margin_width,
		new_interior_height = activewin.height - margin_height;
	//convert viewport x,y to desktop x,y:
	long desktop_x = activewin.x + _viewport.x,
		desktop_y = activewin.y + _viewport.y;

	//moveresize uses exterior for position, but interior for width/height
	config::debug("move: %ldx %ldy %luw %luh - margins %dw %dh + viewport %dx %dy = %ldx %ldy %luw %luh",
			activewin.x, activewin.y, activewin.width, activewin.height,
			margin_width, margin_height, _viewport.x, _viewport.y,
			desktop_x, desktop_y, new_interior_width, new_interior_height);

	unsigned long grflags = 1;//northwest gravity
	grflags |= 0xF00;//mark all x,y,w,h as active
	grflags |= SOURCE_INDICATION << 12;//bits 12-15 for source indication
	return client_msg(disp, *win, "_NET_MOVERESIZE_WINDOW",
			grflags, desktop_x, desktop_y,
			new_interior_width, new_interior_height);
}
