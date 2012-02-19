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

#include "config.h"
#include "neighbor.h"
#include "window.h"
#include "x11-util.h"

#define SOURCE_INDICATION 2 //say that we're a pager or taskbar

namespace {
	int _client_msg(Display* disp, Window win, Atom msg,
			unsigned long data0, unsigned long data1,
			unsigned long data2, unsigned long data3,
			unsigned long data4) {
		XEvent event;
		long mask = SubstructureRedirectMask | SubstructureNotifyMask;

		event.xclient.type = ClientMessage;
		event.xclient.serial = 0;
		event.xclient.send_event = True;
		event.xclient.message_type = msg;
		event.xclient.window = win;
		event.xclient.format = 32;
		event.xclient.data.l[0] = data0;
		event.xclient.data.l[1] = data1;
		event.xclient.data.l[2] = data2;
		event.xclient.data.l[3] = data3;
		event.xclient.data.l[4] = data4;

		DEBUG("send message_type=%s, data=(%lu,%lu,%lu,%lu,%lu)",
				XGetAtomName(disp, msg), data0, data1, data2, data3, data4);

		if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
			return true;
		} else {
			ERROR("Cannot send %s event.", msg);
			return false;
		}
	}

	inline void print_window(Display* disp, Window win) {
		if (!config::debug_enabled) { return; }
		Window root;
		int x, y;
		unsigned int width, height, border, depth;
		if (XGetGeometry(disp, win, &root, &x, &y, &width,
						&height, &border, &depth) == 0) {
			ERROR_DIR("get geometry failed");
			return;
		}
		DEBUG("  %dx %dy %uw %uh %ub", x, y, width, height, border);
	}

	Window* get_active_window(Display* disp) {
		static Atom actwin_msg = XInternAtom(disp, "_NET_ACTIVE_WINDOW", False);
		Window* ret = (Window*)x11_util::get_property(disp, DefaultRootWindow(disp),
				XA_WINDOW, actwin_msg, NULL);
		if (ret == NULL) {
			ERROR_DIR("unable to get active window");
		}
		return ret;
	}

	bool is_dock_window(Display* disp, Window win) {
		/* disallow moving/selecting this window if it has type DESKTOP or DOCK.
		   (avoid messing with the user's desktop components) */
		bool ret = false;
		size_t count = 0;
		static Atom wintype_msg = XInternAtom(disp, "_NET_WM_WINDOW_TYPE", False);
		Atom* types = (Atom*)x11_util::get_property(disp, win, XA_ATOM, wintype_msg, &count);
		if (types == NULL) {
			ERROR_DIR("couldn't get window types");
			//assume window types are fine, keep going
		} else {
			static Atom desktop_type = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DESKTOP", False),
				dock_type = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DOCK", False);
			for (size_t i = 0; i < count; ++i) {
				DEBUG("%d type %lu: %d %s",
						win, i, types[i], XGetAtomName(disp, types[i]));
				if (types[i] == desktop_type || types[i] == dock_type) {
					ret = true;
					if (!config::debug_enabled) {
						break;
					}
				}
			}
			x11_util::free_property(types);
		}
		return ret;
	}

	bool is_menu_window(Display* disp, Window win) {
		/* also disallow moving/selecting this window if it has BOTH SKIP_PAGER and SKIP_TASKBAR.
		   (avoid messing with auxiliary panels and menus) */
		bool ret = false;
		size_t count = 0;
		static Atom state_msg = XInternAtom(disp, "_NET_WM_STATE", False);
		Atom* states = (Atom*)x11_util::get_property(disp, win, XA_ATOM, state_msg, &count);
		if (states == NULL) {
			ERROR_DIR("couldn't get window states");
			//assume window states are fine, keep going
		} else {
			static Atom skip_pager = XInternAtom(disp, "_NET_WM_STATE_SKIP_PAGER", False),
				skip_taskbar = XInternAtom(disp, "_NET_WM_STATE_SKIP_TASKBAR", False);
			bool has_skip_pager = false, has_skip_taskbar = false;
			for (size_t i = 0; i < count; ++i) {
				DEBUG("%d state %lu: %d %s",
						win, i, states[i], XGetAtomName(disp, states[i]));
				if (states[i] == skip_pager) {
					has_skip_pager = true;
				} else if (states[i] == skip_taskbar) {
					has_skip_taskbar = true;
				}
			}
			x11_util::free_property(states);
			if (has_skip_pager && has_skip_taskbar) {
				ret = true;
			}
		}
		return ret;
	}

	bool get_window_size(Display* disp, Window win,
			Dimensions* out_exterior = NULL,
			unsigned int* out_margin_width = NULL,
			unsigned int* out_margin_height = NULL) {
		Window root;
		unsigned int internal_width, internal_height;
		{
			/* first, get the interior width/height from this window
			   (so that we can calculate margins) */
			int x, y;
			unsigned int border, depth;
			if (XGetGeometry(disp, win, &root, &x, &y, &internal_width,
							&internal_height, &border, &depth) == 0) {
				ERROR_DIR("get geometry failed");
				return false;
			}
		}

		if (win == root) {
			ERROR_DIR("this window is root! treating this as an error.");
			return false;
		}

		/* now traverse up the parents until we reach the one JUST BEFORE root,
		   and get the external width/height and x/y from that. */
		Window just_before_root;
		{
			int count = 1;
			Window parent = win;
			Window* children;
			unsigned int children_count;
			do {
				just_before_root = parent;
				if (XQueryTree(disp, just_before_root, &root,
								&parent, &children, &children_count) == 0) {
					ERROR_DIR("get query tree failed");
					return false;
				}
				if (children != NULL) {
					XFree(children);
				}
				DEBUG("%d window=%lu, parent=%lu, root=%lu",
						count, just_before_root, parent, root);
				print_window(disp, just_before_root);
			} while (++count < 50 && parent != root);
		}

		int x, y;
		unsigned int external_width, external_height;
		{
			unsigned int border, depth;
			if (XGetGeometry(disp, just_before_root, &root, &x, &y, &external_width,
							&external_height, &border, &depth) == 0) {
				ERROR_DIR("get geometry failed");
				return false;
			}
		}

		if (out_exterior != NULL) {
			out_exterior->x = x;
			out_exterior->y = y;
			out_exterior->width = external_width;
			out_exterior->height = external_height;
		}

		if (out_margin_width != NULL) {
			*out_margin_width = external_width - internal_width;
		}
		if (out_margin_height != NULL) {
			*out_margin_height = external_height - internal_height;
		}

		DEBUG("size: exterior %uw %uh - interior %uw %uh = margins %dw %dh",
				external_width, external_height,
				internal_width, internal_height,
				external_width - internal_width,
				external_height - internal_height);
		return true;
	}

	bool activate_window(Display* disp, Window curactive, Window newactive) {
		static Atom active_msg = XInternAtom(disp, "_NET_ACTIVE_WINDOW", False);
		if (!_client_msg(disp, newactive, active_msg,
						SOURCE_INDICATION, CurrentTime, curactive, 0, 0)) {
			ERROR_DIR("couldn't activate");
			return false;
		}
		return true;
	}

	bool set_window_state(Display* disp, Window win, Atom state1, Atom state2, bool enable) {
		/*
		  this disagrees with docs, which say that we should be using a
		  _NET_WM_STATE_DISABLE/_ENABLE atom in data[0]. That apparently doesn't
		  work in practice, but '0'/'1' do.
		*/

		int val = (enable) ? 1 : 0;// just to be explicit
		static Atom state_msg = XInternAtom(disp, "_NET_WM_STATE", False);
		return _client_msg(disp, win, state_msg,
				val, state1, state2, SOURCE_INDICATION, 0);
	}

	bool maximize_window(Display* disp, Window win, bool enable) {
		static Atom max_vert = XInternAtom(disp, "_NET_WM_STATE_MAXIMIZED_VERT", False),
			max_horiz = XInternAtom(disp, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
		return set_window_state(disp, win, max_vert, max_horiz, enable);
	}
}

bool window::select_activate(grid::POS dir) {
	Display* disp = XOpenDisplay(NULL);
	if (disp == NULL) {
		ERROR_DIR("unable to get display");
		return false;
	}

	std::vector<Window> wins;

	{
		size_t win_count = 0;
		static Atom clientlist_msg = XInternAtom(disp, "_NET_CLIENT_LIST", False);
		Window* all_wins = (Window*)x11_util::get_property(disp, DefaultRootWindow(disp),
				XA_WINDOW, clientlist_msg, &win_count);
		if (all_wins == NULL || win_count == 0) {
			ERROR_DIR("unable to get list of windows");
			if (all_wins != NULL) {
				x11_util::free_property(all_wins);
			}
			XCloseDisplay(disp);
			return false;
		}
		// only select normal windows, ignore docks and menus
		for (size_t i = 0; i < win_count; ++i) {
			if (!is_dock_window(disp, all_wins[i]) && !is_menu_window(disp, all_wins[i])) {
				wins.push_back(all_wins[i]);
			}
		}
		x11_util::free_property(all_wins);
	}

	size_t active_window = 0;
	dim_list_t all_windows;
	{
		Window* active = get_active_window(disp);
		if (active == NULL) {
			XCloseDisplay(disp);
			return false;
		}
		for (size_t i = 0; i < wins.size(); ++i) {
			if (wins[i] == *active) {
				active_window = i;
				DEBUG_DIR("ACTIVE:");
			}
			all_windows.push_back(Dimensions());
			get_window_size(disp, wins[i], &all_windows.back(), NULL, NULL);
		}
		x11_util::free_property(active);
	}

	size_t next_window;
	neighbor::select(dir, all_windows, active_window, next_window);

	bool ok = activate_window(disp, wins[active_window], wins[next_window]);

	XCloseDisplay(disp);
	return ok;
}

ActiveWindow::~ActiveWindow() {
	if (disp != NULL) {
		XCloseDisplay(disp);
	}
	if (win != NULL) {
		x11_util::free_property(win);
	}
}

bool ActiveWindow::init() {
	if (disp == NULL) {
		disp = XOpenDisplay(NULL);
		if (disp == NULL) {
			ERROR_DIR("unable to get display");
			return false;
		}
	}

	if (win == NULL) {
		win = get_active_window(disp);
		if (win == NULL) {
			return false;
		}
	}
	return true;
}

bool ActiveWindow::Size(Dimensions& activewin) {
	if (!init()) {
		return false;
	}

	if (is_dock_window(disp, *win) || is_menu_window(disp, *win)) {
		LOG_DIR("Active window is a desktop or dock. Ignoring move request.");
		return false;
	}

	if (!get_window_size(disp, *win, &activewin, NULL, NULL)) {
		ERROR_DIR("couldn't get window size");
		return false;
	}

	DEBUG("activewin %dx %dy %luw %luh",
			activewin.x, activewin.y, activewin.width, activewin.height);

	return true;
}

bool ActiveWindow::MoveResize(const Dimensions& activewin) {
	if (!init()) {
		return false;
	}

	unsigned int margin_width, margin_height;
	if (!get_window_size(disp, *win, NULL, &margin_width, &margin_height)) {
		return false;
	}

	//demaximize the window before attempting to move it
	if (!maximize_window(disp, *win, false)) {
		ERROR_DIR("couldn't demaximize");
		//disregard failure
	}

	unsigned long new_interior_width = activewin.width - margin_width,
		new_interior_height = activewin.height - margin_height;

	//moveresize uses exterior for position, but interior for width/height
	DEBUG("%ldx %ldy %luw %luh - margins %dw %dh = %ldx %ldy %luw %luh",
			activewin.x, activewin.y, activewin.width, activewin.height,
			margin_width, margin_height,
			activewin.x, activewin.y, new_interior_width, new_interior_height);

	if (XMoveResizeWindow(disp, *win, activewin.x, activewin.y,
					new_interior_width, new_interior_height) == 0) {
		ERROR("MoveResize to %ldx %ldy %luw %luh failed.",
				activewin.x, activewin.y, new_interior_width, new_interior_height);
		return false;
	}
	return true;
}

bool ActiveWindow::Maximize() {
	if (!init()) {
		return false;
	}

	if (!maximize_window(disp, *win, true)) {
		ERROR_DIR("couldn't maximize");
		return false;
	}
	return true;
}
bool ActiveWindow::DeFullscreen() {
	if (!init()) {
		return false;
	}

	static Atom fs = XInternAtom(disp, "_NET_WM_STATE_FULLSCREEN", False);
	if (!set_window_state(disp, *win, fs, 0, false)) {
		ERROR_DIR("couldn't defullscreen");
		return false;
	}
	return true;
}
bool ActiveWindow::DeShade() {
	if (!init()) {
		return false;
	}

	static Atom shade = XInternAtom(disp, "_NET_WM_STATE_SHADED", False);
	if (!set_window_state(disp, *win, shade, 0, false)) {
		ERROR_DIR("couldn't deshade");
		return false;
	}
	return true;
}
