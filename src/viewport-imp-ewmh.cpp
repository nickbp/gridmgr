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
#include "viewport-imp-ewmh.h"
#include "x11-util.h"

bool viewport::ewmh::get_viewports(Display* disp, const Dimensions& /*activewin*/,
        dim_list_t& viewports_out, size_t& active_out) {
    //get current workspace
    unsigned long cur_workspace;
    {
        unsigned long* cur_ptr;
        static Atom curdesk_msg = XInternAtom(disp, "_NET_CURRENT_DESKTOP", False);
        if (!(cur_ptr = (unsigned long *)x11_util::get_property(disp, DefaultRootWindow(disp),
                                XA_CARDINAL, curdesk_msg, NULL))) {
            ERROR("unable to retrieve current desktop");
            return false;
        }
        cur_workspace = *cur_ptr;
        x11_util::free_property(cur_ptr);
    }

    unsigned long* area;
    size_t area_count = 0;//number of areas returned, one per workspace. each area contains 4 ulongs.
    static Atom workarea_msg = XInternAtom(disp, "_NET_WORKAREA", False);
    if (!(area = (unsigned long*)x11_util::get_property(disp, DefaultRootWindow(disp),
                            XA_CARDINAL, workarea_msg, &area_count))) {
        ERROR("unable to retrieve spanning workarea");
        return false;
    }
    if (area_count == 0) {
        ERROR("unable to retrieve spanning workarea.");
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
    viewports_out.clear();//nice to have
    viewports_out.push_back(Dimensions());
    active_out = 0;

    Dimensions& v = viewports_out.back();
    v.x = area[cur_workspace*4];
    v.y = area[(cur_workspace*4)+1];
    v.width = area[(cur_workspace*4)+2];
    v.height = area[(cur_workspace*4)+3];

    x11_util::free_property(area);
    return true;
}
