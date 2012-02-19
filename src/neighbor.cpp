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
		   point. Eg POS_UP -> "is p above this?" */
		bool direction(grid::POS dir, const point& p) const {
			// note: M_PI_4 == pi/4, from math.h.
			if (y == p.y && x == p.x) {
				// p is on top of us, avoid getting stuck
				return false;
			}
			//DEBUG("%s: %f vs %f", grid::dir_str(dir), abs_atan(p), (M_PI_4));
			switch (dir) {
			case grid::POS_UP_LEFT:
				return (y > p.y && x > p.x);
			case grid::POS_UP_RIGHT:
				return (y > p.y && x < p.x);
			case grid::POS_DOWN_LEFT:
				return (y < p.y && x > p.x);
			case grid::POS_DOWN_RIGHT:
				return (y < p.y && x < p.x);

			case grid::POS_UP_CENTER:
				if (y < p.y) { return false; } // p is below us
				return abs_atan(p) >= M_PI_4; // p is too far right/left of us
			case grid::POS_DOWN_CENTER:
				if (y > p.y) { return false; } // p is above us
				return abs_atan(p) >= M_PI_4; // p is too far right/left of us

			case grid::POS_LEFT:
				if (x < p.x) { return false; } // p is right of us
				return abs_atan(p) <= M_PI_4; // p is too far above/below us
			case grid::POS_RIGHT:
				if (x > p.x) { return false; } // p is left of us
				return abs_atan(p) <= M_PI_4; // p is too far above/below us

			case grid::POS_CENTER:
			case grid::POS_UNKNOWN:
			case grid::POS_CURRENT:
				ERROR("Internal error: invalid dir %s", grid::pos_str(dir));
				break;
			}
			return false;//???
		}

		/* Not a true 'distance', more of a weighting where smaller is better. */
		double distance(grid::POS dir, const point& p) const {
			long w = DISTANCE(p.x, x), h = DISTANCE(p.y, y);
			switch (dir) {
			case grid::POS_UP_LEFT:
			case grid::POS_UP_RIGHT:
			case grid::POS_DOWN_LEFT:
			case grid::POS_DOWN_RIGHT:
				//use a weighting that favors diagonals (where w == h)
			{
				double dist = sqrt((w * w) + (h * h));
				//sqrt again to further decrease dist's importance
				return sqrt(dist) * DISTANCE(w, h);
			}

			case grid::POS_UP_CENTER:
			case grid::POS_DOWN_CENTER:
			case grid::POS_RIGHT:
			case grid::POS_LEFT:
				//use actual distance, diagonals are automatically farther
				return sqrt((w * w) + (h * h));

			case grid::POS_CENTER:
			case grid::POS_UNKNOWN:
			case grid::POS_CURRENT:
				ERROR("Internal error: invalid dir %s", grid::pos_str(dir));
				break;
			}
			return 0;//???
		}

		void shift_pos(grid::POS dir, long max_x, long max_y) {
			switch (dir) {
			case grid::POS_UP_LEFT:
				// we're looking up left, so move the point down right
				y += max_y;
				x += max_x;
				break;
			case grid::POS_DOWN_RIGHT:
				// opposite of UP_LEFT
				shift_pos(grid::POS_UP_LEFT, -1 * max_x, -1 * max_y);
				break;

			case grid::POS_UP_RIGHT:
				// we're looking up right, so move the point down left
				y += max_y;
				x -= max_x;
				break;
			case grid::POS_DOWN_LEFT:
				// opposite of UP_RIGHT
				shift_pos(grid::POS_UP_RIGHT, -1 * max_x, -1 * max_y);
				break;

			case grid::POS_UP_CENTER:
				// we're looking up, so move the point down
				y += max_y;
				break;
			case grid::POS_DOWN_CENTER:
				// opposite of UP_CENTER
				shift_pos(grid::POS_UP_CENTER, -1 * max_x, -1 * max_y);
				break;

			case grid::POS_LEFT:
				// we're looking left, so move the point right
				x += max_x;
				break;
			case grid::POS_RIGHT:
				// opposite of LEFT
				shift_pos(grid::POS_LEFT, -1 * max_x, -1 * max_y);
				break;

			case grid::POS_CENTER:
			case grid::POS_UNKNOWN:
			case grid::POS_CURRENT:
				ERROR("Internal error: invalid dir %s", grid::pos_str(dir));
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
	bool select_nearest_in_direction(grid::POS dir, const std::vector<point>& pts,
			size_t active, size_t& select) {
		// find nearst point that matches the given direction
		double nearest_dist = 0;
		long nearest_i = -1;
		const point& active_pt = pts[active];
		DEBUG("search %lu for points %s of %ld,%ld:",
				pts.size()-1, grid::pos_str(dir), active_pt.x, active_pt.y);
		for (size_t i = 0; i < pts.size(); ++i) {
			if (i == active) {
				DEBUG("skip: %ld,%ld", pts[i].x, pts[i].y);
				continue;
			}

			const point& pt = pts[i];
			if (active_pt.direction(dir, pt)) {
				double dist = active_pt.distance(dir, pt);
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
	grid::POS fallback_direction(grid::POS dir) {
		// go clockwise
		switch (dir) {
		case grid::POS_UP_CENTER:
			return grid::POS_UP_RIGHT;
		case grid::POS_UP_RIGHT:
			return grid::POS_RIGHT;
		case grid::POS_RIGHT:
			return grid::POS_DOWN_RIGHT;
		case grid::POS_DOWN_RIGHT:
			return grid::POS_DOWN_CENTER;
		case grid::POS_DOWN_CENTER:
			return grid::POS_DOWN_LEFT;
		case grid::POS_DOWN_LEFT:
			return grid::POS_LEFT;
		case grid::POS_LEFT:
			return grid::POS_UP_LEFT;
		case grid::POS_UP_LEFT:
			return grid::POS_UP_CENTER;
		case grid::POS_CENTER:
		case grid::POS_UNKNOWN:
		case grid::POS_CURRENT:
			ERROR("Internal error: invalid dir %s", grid::pos_str(dir));
			break;
		}
		return grid::POS_UP_CENTER;//???
	}
}

#define MAX_X(dim) (dim.x + dim.width)
#define MAX_Y(dim) (dim.y + dim.height)

void neighbor::select(grid::POS dir, const dim_list_t& all, size_t active, size_t& select) {
	if (all.size() <= 1) {
		select = 0;
		return;
	}
	if (dir == grid::POS_CURRENT) {
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
