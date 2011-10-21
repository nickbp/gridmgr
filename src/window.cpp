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
#include "viewport.h"
#include "x11-util.h"
#include "config.h"

#define SOURCE_INDICATION 2 //say that we're a pager or taskbar

namespace {
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
		XWindowAttributes attr;
		if (XGetWindowAttributes(disp, win, &attr) == 0) {
			ERROR_DIR("get geometry failed");
			return false;
		}

		int interior_x, interior_y;
		Window tmp;
		if (XTranslateCoordinates(disp, win, attr.root, 0, 0,
						&interior_x, &interior_y, &tmp) == 0) {
			ERROR_DIR("coordinate transform failed");
			return false;
		}

		DEBUG("pos: interior %dx %dy - attr %dx %dy %dw %dh %ub = exterior %dx %dy %dw %dh",
				interior_x, interior_y,
				attr.x, attr.y, attr.width, attr.height, attr.border_width,
				interior_x - attr.x, interior_y - attr.y,
				attr.width + attr.x + 2*attr.border_width,
				attr.height + attr.y + 2*attr.border_width);

		if (out_exterior != NULL) {
			out_exterior->x = interior_x - attr.x;
			out_exterior->y = interior_y - attr.y;
			out_exterior->width = attr.width + attr.x + 2*attr.border_width;
			out_exterior->height = attr.height + attr.y + 2*attr.border_width;
		}

		if (out_margin_width != NULL) {
			*out_margin_width = attr.x + 2*attr.border_width;
		}
		if (out_margin_height != NULL) {
			*out_margin_height = attr.y + 2*attr.border_width;
		}

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

	bool get_viewport(Display* disp, const ActiveWindow::Dimensions& activewin,
			ActiveWindow::Dimensions& viewport_out) {
#ifdef USE_XINERAMA
		//try xinerama, fall back to ewmh if xinerama is unavailable
		return viewport::get_viewport_xinerama(disp, activewin, viewport_out) ||
			viewport::get_viewport_ewmh(disp, viewport_out);
#else
		//xinerama disabled; just do ewmh
		return viewport::get_viewport_ewmh(disp, viewport_out);
#endif
	}
}

ActiveWindow::ActiveWindow() {
	if (!(disp = XOpenDisplay(NULL))) {
		ERROR_DIR("unable to get display");
		return;
	}

	if (!(win = (Window*)x11_util::get_property(disp, DefaultRootWindow(disp),
							XA_WINDOW, "_NET_ACTIVE_WINDOW", NULL))) {
		ERROR_DIR("unable to get active window");
	}
}

ActiveWindow::~ActiveWindow() {
	if (disp != NULL) {
		XCloseDisplay(disp);
	}
	if (win != NULL) {
		x11_util::free_property(win);
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
		if (!(states = (Atom*)x11_util::get_property(disp, *win,
								XA_ATOM, "_NET_WM_STATE", &count))) {
			ERROR_DIR("couldnt get window states");
		} else {
			for (size_t i = 0; i < count; ++i) {
				LOG("state %lu: %d %s",
						i, states[i], XGetAtomName(disp, states[i]));
			}
			x11_util::free_property(states);
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
