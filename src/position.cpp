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

#include <math.h> // round()

#include "config.h"
#include "position.h"

namespace {
	inline const char* mode_str(grid::MODE mode) {
		switch (mode) {
		case grid::MODE_UNKNOWN:
			return "UNKNOWN";
		case grid::MODE_TWO_COL:
			return "TWO_COL";
		case grid::MODE_THREE_COL_S:
			return "THREE_COL_S";
		case grid::MODE_THREE_COL_L:
			return "THREE_COL_L";
		default:
			break;
		}
		return "???";
	}

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
	// whether two numbers are 'near' one another, according to a fudge factor.
	template <typename A, typename B>
	inline bool _near(A a, B b, const char* a_desc, const char* b_desc) {
		// fudge factor: the greater of 5px or 5% of the greater number.
		double fudge = MAX(5, 0.05 * MAX(a,b));
		bool ret = (a + fudge) >= b && (a - fudge) <= b;
		DEBUG("%s(%d) vs %s(%d): %s",
				a_desc, (int)a, b_desc, (int)b,
				(ret) ? "true" : "false");
		return ret;
	}
#define NEAR(a,b) _near(a, b, #a, #b)
}

/* given window's dimensions, estimate its state (or unknown+unknown)
   (inverse of StateToDim) */
bool PositionCalc::CurState(const Dimensions& viewport, State& out) const {
	//get window x/y relative to viewport x/y
	int rel_x = window.x - viewport.x,
		rel_y = window.y - viewport.y;
	out.pos = grid::POS_UNKNOWN;
	out.mode = grid::MODE_UNKNOWN;
	if (NEAR(window.width, viewport.width / 2.)) {
		if (NEAR(window.height, viewport.height / 2.)) {
			if (NEAR(rel_x, 0)) {
				if (NEAR(rel_y, 0)) {
					// top left quadrant
					out.pos = grid::POS_UP_LEFT;
					out.mode = grid::MODE_TWO_COL;
				} else if (NEAR(rel_y, viewport.height / 2.)) {
					// bottom left quadrant
					out.pos = grid::POS_DOWN_LEFT;
					out.mode = grid::MODE_TWO_COL;
				}
			} else if (NEAR(rel_x, viewport.width / 2.)) {
				if (NEAR(rel_y, 0)) {
					// top right quadrant
					out.pos = grid::POS_UP_RIGHT;
					out.mode = grid::MODE_TWO_COL;
				} else if (NEAR(rel_y, viewport.height / 2.)) {
					// bottom right quadrant
					out.pos = grid::POS_DOWN_RIGHT;
					out.mode = grid::MODE_TWO_COL;
				}
			}
		} else if (NEAR(window.height, viewport.height) &&
				NEAR(rel_y, 0)) {
			if (NEAR(rel_x, 0)) {
				// left half
				out.pos = grid::POS_LEFT;
				out.mode = grid::MODE_TWO_COL;
			} else if (NEAR(rel_x, viewport.width / 2.)) {
				// right half
				out.pos = grid::POS_RIGHT;
				out.mode = grid::MODE_TWO_COL;
			}
		}
	} else if (NEAR(window.width, viewport.width / 3.)) {
		if (NEAR(window.height, viewport.height / 2.)) {
			if (NEAR(rel_x, 0)) {
				if (NEAR(rel_y, 0)) {
					// top left col
					out.pos = grid::POS_UP_LEFT;
					out.mode = grid::MODE_THREE_COL_S;
				} if (NEAR(rel_y, viewport.height / 2.)) {
					// bottom left col
					out.pos = grid::POS_DOWN_LEFT;
					out.mode = grid::MODE_THREE_COL_S;
				}
			} else if (NEAR(rel_x, viewport.width / 3.)) {
				if (NEAR(rel_y, 0)) {
					// top center col
					out.pos = grid::POS_UP_CENTER;
					out.mode = grid::MODE_THREE_COL_S;
				} else if (NEAR(rel_y, viewport.height / 2.)) {
					// bottom center col
					out.pos = grid::POS_DOWN_CENTER;
					out.mode = grid::MODE_THREE_COL_S;
				}
			} else if (NEAR(rel_x, 2 * viewport.width / 3.)) {
				if (NEAR(rel_y, 0)) {
					// top right col
					out.pos = grid::POS_UP_RIGHT;
					out.mode = grid::MODE_THREE_COL_S;
				} else if (NEAR(rel_y, viewport.height / 2.)) {
					// bottom right col
					out.pos = grid::POS_DOWN_RIGHT;
					out.mode = grid::MODE_THREE_COL_S;
				}
			}
		} else if (NEAR(window.height, viewport.height) &&
				NEAR(rel_y, 0)) {
			if (NEAR(rel_x, 0)) {
				// left col
				out.pos = grid::POS_LEFT;
				out.mode = grid::MODE_THREE_COL_S;
			} else if (NEAR(rel_x, viewport.width / 3.)) {
				// center col
				out.pos = grid::POS_CENTER;
				out.mode = grid::MODE_THREE_COL_S;
			} else if (NEAR(rel_x, 2 * viewport.width / 3.)) {
				// right col
				out.pos = grid::POS_RIGHT;
				out.mode = grid::MODE_THREE_COL_S;
			}
		}
	} else if (NEAR(window.width, 2 * viewport.width / 3.)) {
		if (NEAR(window.height, viewport.height / 2.)) {
			if (NEAR(rel_x, 0)) {
				if (NEAR(rel_y, 0)) {
					// top left two cols
					out.pos = grid::POS_UP_LEFT;
					out.mode = grid::MODE_THREE_COL_L;
				} else if (NEAR(rel_y, viewport.height / 2.)) {
					// bottom left two cols
					out.pos = grid::POS_DOWN_LEFT;
					out.mode = grid::MODE_THREE_COL_L;
				}
			} else if (NEAR(rel_x, viewport.width / 3.)) {
				if (NEAR(rel_y, 0)) {
					// top right two cols
					out.pos = grid::POS_UP_RIGHT;
					out.mode = grid::MODE_THREE_COL_L;
				} else if (NEAR(rel_y, viewport.height / 2.)) {
					// bottom right two cols
					out.pos = grid::POS_DOWN_RIGHT;
					out.mode = grid::MODE_THREE_COL_L;
				}
			}
		} else if (NEAR(window.height, viewport.height) &&
				NEAR(rel_y, 0)) {
			if (NEAR(rel_x, 0)) {
				// left two cols
				out.pos = grid::POS_LEFT;
				out.mode = grid::MODE_THREE_COL_L;
			} else if (NEAR(rel_x, viewport.width / 3.)) {
				// right two cols
				out.pos = grid::POS_RIGHT;
				out.mode = grid::MODE_THREE_COL_L;
			}
		}
	} else if (NEAR(window.width, viewport.width) &&
			NEAR(rel_x, 0)) {
		if (NEAR(window.height, viewport.height / 2.)) {
			if (NEAR(rel_y, 0)) {
				// top half
				out.pos = grid::POS_UP_CENTER;
				out.mode = grid::MODE_THREE_COL_L;
			} else if (NEAR(rel_y, viewport.height / 2.)) {
				// bottom half
				out.pos = grid::POS_DOWN_CENTER;
				out.mode = grid::MODE_THREE_COL_L;
			}
		} else if (NEAR(window.height, viewport.height) &&
				NEAR(rel_y, 0)) {
			// full screen
			out.pos = grid::POS_CENTER;
			out.mode = grid::MODE_THREE_COL_L;
		}
	}

	DEBUG("%ldx %ldy %luw %luh -> pos=%s mode=%s",
			rel_x, rel_y, window.width, window.height,
			pos_str(out.pos), mode_str(out.mode));
	return true;
}

bool PositionCalc::NextState(const State& cur, grid::POS req_pos, State& out) const {
	if (req_pos == grid::POS_UNKNOWN) {// nice to have
		ERROR("Position '%s' was requested. Internal error?", pos_str(req_pos));
		return false;
	}
	if (req_pos == grid::POS_CURRENT) {
		out = cur;
	} else if (cur.pos == req_pos) {
		// position is same, so rotate mode
		out.pos = cur.pos;
		switch (cur.pos) {
		case grid::POS_UP_CENTER:
		case grid::POS_CENTER:
		case grid::POS_DOWN_CENTER:
			// for center col: 3x2L -> 3x2S (-> 3x2L) (no 2x2)
			switch (cur.mode) {
			case grid::MODE_THREE_COL_L:
				out.mode = grid::MODE_THREE_COL_S;
				break;
			case grid::MODE_THREE_COL_S:
			default:
				out.mode = grid::MODE_THREE_COL_L;
			}
			break;
		default:
			// for everything else: 2x2 -> 3x2L -> 3x2S (-> 2x2)
			switch (cur.mode) {
			case grid::MODE_UNKNOWN:
			case grid::MODE_THREE_COL_L:
				out.mode = grid::MODE_THREE_COL_S;
				break;
			case grid::MODE_TWO_COL:
				out.mode = grid::MODE_THREE_COL_L;
				break;
			case grid::MODE_THREE_COL_S:
			default:
				out.mode = grid::MODE_TWO_COL;
			}
		}
	} else {
		// new position, so start with initial mode
		out.pos = req_pos;
		switch (req_pos) {
		case grid::POS_UP_CENTER:
		case grid::POS_CENTER:
		case grid::POS_DOWN_CENTER:
			// for center col: start with 3x2L
			out.mode = grid::MODE_THREE_COL_L;
			break;
		default:
			// for everything else: start with 2x2
			out.mode = grid::MODE_TWO_COL;
		}
	}
	DEBUG("curpos=%s curmode=%s + reqpos=%s -> pos=%s mode=%s",
			pos_str(cur.pos), mode_str(cur.mode), pos_str(req_pos),
			pos_str(out.pos), mode_str(out.mode));
	return true;
}

/* given window's state, calculate its dimensions.
   could have some kind of fancy autogeneration here,
   but there's a very finite number of possible positions (for now?) */
bool PositionCalc::StateToDim(const Dimensions& viewport, const State& state,
		Dimensions& out) const {
	bool ret = true;
	long rel_x = 0, rel_y = 0;//coordinates relative to viewport
	switch (state.mode) {
	case grid::MODE_TWO_COL:
		switch (state.pos) {
		case grid::POS_UP_LEFT:// top left quadrant
			rel_x = 0;
			rel_y = 0;
			out.width = viewport.width / 2.;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_UP_CENTER:// invalid, use mode THREE_COL_S/L
			return false;
		case grid::POS_UP_RIGHT:// top right quadrant
			rel_x = viewport.width / 2.;
			rel_y = 0;
			out.width = viewport.width / 2.;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_LEFT:// left half
			rel_x = 0;
			rel_y = 0;
			out.width = viewport.width / 2.;
			out.height = viewport.height;
			break;
		case grid::POS_CENTER:// invalid, use mode THREE_COL_S/L
			return false;
		case grid::POS_RIGHT:// right half
			rel_x = viewport.width / 2.;
			rel_y = 0;
			out.width = viewport.width / 2.;
			out.height = viewport.height;
			break;
		case grid::POS_DOWN_LEFT:// bottom left quadrant
			rel_x = 0;
			rel_y = viewport.height / 2.;
			out.width = viewport.width / 2.;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_DOWN_CENTER:// invalid, use mode THREE_COL_S/L
			return false;
		case grid::POS_DOWN_RIGHT:// bottom right quadrant
			rel_x = viewport.width / 2.;
			rel_y = viewport.height / 2.;
			out.width = viewport.width / 2.;
			out.height = viewport.height / 2.;
			break;
		default:
			ret = false;
			break;
		}
		break;
	case grid::MODE_THREE_COL_S:
		switch (state.pos) {
		case grid::POS_UP_LEFT:// top left col
			rel_x = 0;
			rel_y = 0;
			out.width = viewport.width / 3.;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_UP_CENTER:// top center col
			rel_x = viewport.width / 3.;
			rel_y = 0;
			out.width = viewport.width / 3.;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_UP_RIGHT:// top right col
			rel_x = 2 * viewport.width / 3.;
			rel_y = 0;
			out.width = viewport.width / 3.;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_LEFT:// left third
			rel_x = 0;
			rel_y = 0;
			out.width = viewport.width / 3.;
			out.height = viewport.height;
			break;
		case grid::POS_CENTER:// center col
			rel_x = viewport.width / 3.;
			rel_y = 0;
			out.width = viewport.width / 3.;
			out.height = viewport.height;
			break;
		case grid::POS_RIGHT:// right third
			rel_x = 2 * viewport.width / 3.;
			rel_y = 0;
			out.width = viewport.width / 3;
			out.height = viewport.height;
			break;
		case grid::POS_DOWN_LEFT:// bottom left col
			rel_x = 0;
			rel_y = viewport.height / 2.;
			out.width = viewport.width / 3.;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_DOWN_CENTER:// bottom center col
			rel_x = viewport.width / 3.;
			rel_y = viewport.height / 2.;
			out.width = viewport.width / 3.;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_DOWN_RIGHT:// bottom right col
			rel_x = 2 * viewport.width / 3.;
			rel_y = viewport.height / 2.;
			out.width = viewport.width / 3.;
			out.height = viewport.height / 2.;
			break;
		default:
			ret = false;
		}
		break;
	case grid::MODE_THREE_COL_L:
		switch (state.pos) {
		case grid::POS_UP_LEFT:// top left two cols
			rel_x = 0;
			rel_y = 0;
			out.width = 2 * viewport.width / 3.;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_UP_CENTER:// top half
			rel_x = 0;
			rel_y = 0;
			out.width = viewport.width;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_UP_RIGHT:// top right two cols
			rel_x = viewport.width / 3.;
			rel_y = 0;
			out.width = 2 * viewport.width / 3.;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_LEFT:// left two thirds
			rel_x = 0;
			rel_y = 0;
			out.width = 2 * viewport.width / 3.;
			out.height = viewport.height;
			break;
		case grid::POS_CENTER:// full screen
			rel_x = 0;
			rel_y = 0;
			out.width = viewport.width;
			out.height = viewport.height;
			break;
		case grid::POS_RIGHT:// right two thirds
			rel_x = viewport.width / 3.;
			rel_y = 0;
			out.width = 2 * viewport.width / 3.;
			out.height = viewport.height;
			break;
		case grid::POS_DOWN_LEFT:// bottom left two cols
			rel_x = 0;
			rel_y = viewport.height / 2.;
			out.width = 2 * viewport.width / 3.;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_DOWN_CENTER:// bottom half
			rel_x = 0;
			rel_y = viewport.height / 2.;
			out.width = viewport.width;
			out.height = viewport.height / 2.;
			break;
		case grid::POS_DOWN_RIGHT:// bottom right two cols
			rel_x = viewport.width / 3.;
			rel_y = viewport.height / 2.;
			out.width = 2 * viewport.width / 3.;
			out.height = viewport.height / 2.;
			break;
		default:
			ret = false;
		}
		break;
	default:
		ret = false;
	}
	if (ret) {
		//convert relative pos to absolute:
		out.x = rel_x + viewport.x;
		out.y = rel_y + viewport.y;
		DEBUG("pos=%s mode=%s -> %ldx %ldy %luw %luh",
				pos_str(state.pos), mode_str(state.mode),
				out.x, out.y, out.width, out.height);
	} else {
		ERROR("Bad pos=%s + mode=%s", pos_str(state.pos), mode_str(state.mode));
	}
	return ret;
}

void PositionCalc::ViewportToDim(const Dimensions& cur_viewport,
		const Dimensions& next_viewport, Dimensions& out) const {
	// just do an exact scaling to the new viewport
	if (cur_viewport.width == 0 || cur_viewport.height == 0) {//nice to have, avoid div0
		out = next_viewport;//just throw something together and get out
		return;
	}
	const double ratio_x = next_viewport.width / (double)cur_viewport.width,
		ratio_y = next_viewport.height / (double)cur_viewport.height;

	// avoid implicit floor to avoid cumulative rounding error:
	out.x = round((window.x - cur_viewport.x) * ratio_x + next_viewport.x);
	out.y = round((window.y - cur_viewport.y) * ratio_y + next_viewport.y);
	out.width = round(window.width * ratio_x);
	out.height = round(window.height * ratio_y);
}
