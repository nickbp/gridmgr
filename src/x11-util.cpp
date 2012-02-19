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
#include "x11-util.h"

#define MAX_PROPERTY_VALUE_LEN 4096

unsigned char* x11_util::get_property(Display *disp, Window win,
		Atom xa_prop_type, Atom xa_prop_name, size_t* out_count) {
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
		ERROR("Cannot get property %d/%s.", xa_prop_name, XGetAtomName(disp, xa_prop_name));
		return NULL;
	} else {
		DEBUG("Property %d/%s -> %lu items", xa_prop_name, XGetAtomName(disp, xa_prop_name), ret_nitems);
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
			ERROR("Invalid type of property %d/%s: req %s, got %s",
					xa_prop_name, XGetAtomName(disp, xa_prop_name), req, got);
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

void x11_util::free_property(void* prop) {
	XFree(prop);
}
