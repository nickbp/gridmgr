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

#include <math.h>

#include "config.h"
#include "neighbor.h"

#define MIDPOINT(min, size) ((size / 2.) + min)
#define DISTANCE(a,b) ((a > b) ? (a - b) : (b - a))

namespace {
	class point {
	public:
		point() : x(0), y(0) { }
		point(const Dimensions& d)
			: x(MIDPOINT(d.x, d.width)),
			  y(MIDPOINT(d.y, d.height)) { }

		/* Returns whether 'p' is in the specified direction relative to this
		   point. Eg DIR_UP -> "is p above this?" */
		bool direction(grid::DIR dir, const point& p) const {
			// note: M_PI_4 == pi/4, from math.h.
			if (y == p.y && x == p.x) { return false; } // p is on top of us
			//DEBUG("%s: %f vs %f", grid::dir_str(dir), abs_atan(p), (M_PI_4));
			switch (dir) {
			case grid::DIR_UP:
				if (y < p.y) { return false; } // p is below us
				return abs_atan(p) >= M_PI_4; // p is too far right/left of us
			case grid::DIR_DOWN:
				if (y > p.y) { return false; } // p is above us
				return abs_atan(p) >= M_PI_4; // p is too far right/left of us

			case grid::DIR_LEFT:
				if (x < p.x) { return false; } // p is right of us
				return abs_atan(p) <= M_PI_4; // p is too far above/below us
			case grid::DIR_RIGHT:
				if (x > p.x) { return false; } // p is left of us
				return abs_atan(p) <= M_PI_4; // p is too far above/below us

			case grid::DIR_UNKNOWN:
			case grid::DIR_CURRENT:
				break;
			}
			return false;//???
		}

		double distance(const point& p) const {
			long w = p.x - x, h = p.y - y;
			return sqrt((w * w) + (h * h));
		}

		void shift_pos(grid::DIR dir, long max_x, long max_y) {
			switch (dir) {
			case grid::DIR_UP:
				// we're looking up, so move the point down
				y += max_y;
				break;
			case grid::DIR_DOWN:
				// opposite of DIR_UP
				shift_pos(grid::DIR_UP, -1 * max_x, -1 * max_y);
				break;

			case grid::DIR_LEFT:
				// we're looking left, so move the point right
				x += max_x;
				break;
			case grid::DIR_RIGHT:
				// opposite of DIR_LEFT
				shift_pos(grid::DIR_LEFT, -1 * max_x, -1 * max_y);
				break;

			case grid::DIR_UNKNOWN:
			case grid::DIR_CURRENT:
				break;//???
			}
		}

		long x;
		long y;

	private:
		inline double abs_atan(const point& p2) const {
			long denom = DISTANCE(x, p2.x);
			if (denom == 0) {
				return M_PI_2;// avoid div0 (pi/2, from math.h)
			} else {
				return atan(DISTANCE(y, p2.y) / (double)denom);
			}
		}
	};

	/* Finds and selects the nearest non-active point in the given direction and
	   returns true, or returns false if none was found. */
	bool select_nearest_in_direction(grid::DIR dir, const std::vector<point>& pts,
			size_t active, size_t& select) {
		// find nearst point that matches the given direction
		double nearest_dist = 0;
		long nearest_i = -1;
		const point& active_pt = pts[active];
		DEBUG("search %lu for points %s of %ld,%ld:", pts.size()-1, grid::dir_str(dir), active_pt.x, active_pt.y);
		for (size_t i = 0; i < pts.size(); ++i) {
			if (i == active) {
				DEBUG("skip: %ld,%ld", pts[i].x, pts[i].y);
				continue;
			}

			const point& pt = pts[i];
			if (active_pt.direction(dir, pt)) {
				double dist = active_pt.distance(pt);
				DEBUG("match!: %ld,%ld (dist %.02f)", pts[i].x, pts[i].y, dist);
				if (nearest_i < 0 || dist < nearest_dist) {
					nearest_i = i;
					nearest_dist = dist;
				}
			} else {
				DEBUG("miss: %ld,%ld", pts[i].x, pts[i].y);
			}
		}

		if (nearest_i >= 0) {
			// found!
			select = nearest_i;
			return true;
		}
		return false;
	}

	/* When nothing is found in a given direction, this function determines what
	   the fallback direction ordering should be */
	grid::DIR fallback_direction(grid::DIR dir) {
		// go clockwise
		switch (dir) {
		case grid::DIR_UP:
			return grid::DIR_RIGHT;
		case grid::DIR_RIGHT:
			return grid::DIR_DOWN;
		case grid::DIR_DOWN:
			return grid::DIR_LEFT;
		case grid::DIR_LEFT:
			return grid::DIR_UP;
		case grid::DIR_UNKNOWN:
		case grid::DIR_CURRENT:
			break;
		}
		return grid::DIR_UP;//???
	}
}

#define MAX_X(dim) (dim.x + dim.width)
#define MAX_Y(dim) (dim.y + dim.height)

void neighbor::select(grid::DIR dir, const dim_list_t& all, size_t active, size_t& select) {
	if (all.size() <= 1) {
		select = 0;
		return;
	}
	if (dir == grid::DIR_CURRENT) {
		select = active;
		return;
	}

	// dimension -> center point and max window dimensions
	size_t bound_x = MAX_X(all[0]), bound_y = MAX_Y(all[0]);
	std::vector<point> pts;
	pts.reserve(all.size());
	for (dim_list_t::const_iterator iter = all.begin();
		 iter != all.end(); ++iter) {
		const Dimensions& d = *iter;
		size_t d_x = MAX_X(d), d_y = MAX_Y(d);
		if (d_x > bound_x) { bound_x = d_x; }
		if (d_y > bound_y) { bound_y = d_y; }
		pts.push_back(point(d));
	}

	for (size_t i = 0; i < 4; ++i) {//try each of the four directions
		if (select_nearest_in_direction(dir, pts, active, select)) {
			return;
		}
		// not found. try shifting active pt for a wraparound search
		pts[active].shift_pos(dir, bound_x, bound_y);
		if (select_nearest_in_direction(dir, pts, active, select)) {
			return;
		}
		// still not found. undo shift and try next direction
		pts[active].shift_pos(dir, -1 * bound_x, -1 * bound_y);
		dir = fallback_direction(dir);
	}

	// STILL not found. just give up!
	select = active;
}
