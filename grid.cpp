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

#include "grid.h"
#include "config.h"
#include "window.h"

namespace grid {
	enum MODE {
		MODE_UNKNOWN,
		MODE_TWO_COL,// 2x2 (not applicable for center col positions)
		MODE_THREE_COL_S,// 3x2 small: each position filling 1 column
		MODE_THREE_COL_L// 3x2 large: sides filling 2 columns and center filling full width
	};
	inline const char* mode_str(MODE mode) {
		switch (mode) {
		case MODE_UNKNOWN:
			return "UNKNOWN";
		case MODE_TWO_COL:
			return "TWO_COL";
		case MODE_THREE_COL_S:
			return "THREE_COL_S";
		case MODE_THREE_COL_L:
			return "THREE_COL_L";
		default:
			break;
		}
		return "???";
	}

	struct Position {
		POS pos;
		MODE mode;
	};

	// given window's position/mode, calculate its dimensions.
	// could have some kind of fancy autogeneration here,
	// but there's a very finite number of possible positions (for now)
	bool calculate_pos(const ActiveWindow::Dimensions& viewport,
			const Position& pos, ActiveWindow::Dimensions& window_out) {
		bool ret = true;
		long rel_x = 0, rel_y = 0;//coordinates relative to viewport
		switch (pos.mode) {
		case MODE_TWO_COL:
			switch (pos.pos) {
			case POS_TOP_LEFT:// top left quadrant
				rel_x = 0;
				rel_y = 0;
				window_out.width = viewport.width / 2.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_TOP_CENTER:// invalid, use mode THREE_COL_S/L
				return false;
			case POS_TOP_RIGHT:// top right quadrant
				rel_x = viewport.width / 2.;
				rel_y = 0;
				window_out.width = viewport.width / 2.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_LEFT:// left half
				rel_x = 0;
				rel_y = 0;
				window_out.width = viewport.width / 2.;
				window_out.height = viewport.height;
				break;
			case POS_CENTER:// invalid, use mode THREE_COL_S/L
				return false;
			case POS_RIGHT:// right half
				rel_x = viewport.width / 2.;
				rel_y = 0;
				window_out.width = viewport.width / 2.;
				window_out.height = viewport.height;
				break;
			case POS_BOT_LEFT:// bottom left quadrant
				rel_x = 0;
				rel_y = viewport.height / 2.;
				window_out.width = viewport.width / 2.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_BOT_CENTER:// invalid, use mode THREE_COL_S/L
				return false;
			case POS_BOT_RIGHT:// bottom right quadrant
				rel_x = viewport.width / 2.;
				rel_y = viewport.height / 2.;
				window_out.width = viewport.width / 2.;
				window_out.height = viewport.height / 2.;
				break;
			default:
				ret = false;
				break;
			}
			break;
		case MODE_THREE_COL_S:
			switch (pos.pos) {
			case POS_TOP_LEFT:// top left col
				rel_x = 0;
				rel_y = 0;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_TOP_CENTER:// top center col
				rel_x = viewport.width / 3.;
				rel_y = 0;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_TOP_RIGHT:// top right col
				rel_x = 2 * viewport.width / 3.;
				rel_y = 0;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_LEFT:// left third
				rel_x = 0;
				rel_y = 0;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height;
				break;
			case POS_CENTER:// center col
				rel_x = viewport.width / 3.;
				rel_y = 0;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height;
				break;
			case POS_RIGHT:// right third
				rel_x = 2 * viewport.width / 3.;
				rel_y = 0;
				window_out.width = viewport.width / 3;
				window_out.height = viewport.height;
				break;
			case POS_BOT_LEFT:// bottom left col
				rel_x = 0;
				rel_y = viewport.height / 2.;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_BOT_CENTER:// bottom center col
				rel_x = viewport.width / 3.;
				rel_y = viewport.height / 2.;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_BOT_RIGHT:// bottom right col
				rel_x = 2 * viewport.width / 3.;
				rel_y = viewport.height / 2.;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			default:
				ret = false;
			}
			break;
		case MODE_THREE_COL_L:
			switch (pos.pos) {
			case POS_TOP_LEFT:// top left two cols
				rel_x = 0;
				rel_y = 0;
				window_out.width = 2 * viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_TOP_CENTER:// top half
				rel_x = 0;
				rel_y = 0;
				window_out.width = viewport.width;
				window_out.height = viewport.height / 2.;
				break;
			case POS_TOP_RIGHT:// top right two cols
				rel_x = viewport.width / 3.;
				rel_y = 0;
				window_out.width = 2 * viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_LEFT:// left two thirds
				rel_x = 0;
				rel_y = 0;
				window_out.width = 2 * viewport.width / 3.;
				window_out.height = viewport.height;
				break;
			case POS_CENTER:// full screen
				rel_x = 0;
				rel_y = 0;
				window_out.width = viewport.width;
				window_out.height = viewport.height;
				break;
			case POS_RIGHT:// right two thirds
				rel_x = viewport.width / 3.;
				rel_y = 0;
				window_out.width = 2 * viewport.width / 3.;
				window_out.height = viewport.height;
				break;
			case POS_BOT_LEFT:// bottom left two cols
				rel_x = 0;
				rel_y = viewport.height / 2.;
				window_out.width = 2 * viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_BOT_CENTER:// bottom half
				rel_x = 0;
				rel_y = viewport.height / 2.;
				window_out.width = viewport.width;
				window_out.height = viewport.height / 2.;
				break;
			case POS_BOT_RIGHT:// bottom right two cols
				rel_x = viewport.width / 3.;
				rel_y = viewport.height / 2.;
				window_out.width = 2 * viewport.width / 3.;
				window_out.height = viewport.height / 2.;
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
			window_out.x = rel_x + viewport.x;
			window_out.y = rel_y + viewport.y;
			DEBUG("pos=%s mode=%s -> %ldx %ldy %luw %luh",
					pos_str(pos.pos), mode_str(pos.mode),
					window_out.x, window_out.y,
					window_out.width, window_out.height);
		} else {
			ERROR("bad pos %s + mode %s", pos_str(pos.pos), mode_str(pos.mode));
		}
		return ret;
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

	// given window's dimensions, estimate its position/mode (or unknown+unknown)
	// (inverse of calculate_pos)
	bool get_current_pos(const ActiveWindow::Dimensions& window,
			const ActiveWindow::Dimensions& viewport, Position& out) {
		//get window x/y relative to viewport x/y
		int rel_x = window.x - viewport.x,
			rel_y = window.y - viewport.y;
		out.pos = POS_UNKNOWN;
		out.mode = MODE_UNKNOWN;
		if (NEAR(window.width, viewport.width / 2.)) {
			if (NEAR(window.height, viewport.height / 2.)) {
				if (NEAR(rel_x, 0)) {
					if (NEAR(rel_y, 0)) {
						// top left quadrant
						out.pos = POS_TOP_LEFT;
						out.mode = MODE_TWO_COL;
					} else if (NEAR(rel_y, viewport.height / 2.)) {
						// bottom left quadrant
						out.pos = POS_BOT_LEFT;
						out.mode = MODE_TWO_COL;
					}
				} else if (NEAR(rel_x, viewport.width / 2.)) {
					if (NEAR(rel_y, 0)) {
						// top right quadrant
						out.pos = POS_TOP_RIGHT;
						out.mode = MODE_TWO_COL;
					} else if (NEAR(rel_y, viewport.height / 2.)) {
						// bottom right quadrant
						out.pos = POS_BOT_RIGHT;
						out.mode = MODE_TWO_COL;
					}
				}
			} else if (NEAR(window.height, viewport.height) &&
					NEAR(rel_y, 0)) {
				if (NEAR(rel_x, 0)) {
					// left half
					out.pos = POS_LEFT;
					out.mode = MODE_TWO_COL;
				} else if (NEAR(rel_x, viewport.width / 2.)) {
					// right half
					out.pos = POS_RIGHT;
					out.mode = MODE_TWO_COL;
				}
			}
		} else if (NEAR(window.width, viewport.width / 3.)) {
			if (NEAR(window.height, viewport.height / 2.)) {
				if (NEAR(rel_x, 0)) {
					if (NEAR(rel_y, 0)) {
						// top left col
						out.pos = POS_TOP_LEFT;
						out.mode = MODE_THREE_COL_S;
					} if (NEAR(rel_y, viewport.height / 2.)) {
						// bottom left col
						out.pos = POS_BOT_LEFT;
						out.mode = MODE_THREE_COL_S;
					}
				} else if (NEAR(rel_x, viewport.width / 3.)) {
					if (NEAR(rel_y, 0)) {
						// top center col
						out.pos = POS_TOP_CENTER;
						out.mode = MODE_THREE_COL_S;
					} else if (NEAR(rel_y, viewport.height / 2.)) {
						// bottom center col
						out.pos = POS_BOT_CENTER;
						out.mode = MODE_THREE_COL_S;
					}
				} else if (NEAR(rel_x, 2 * viewport.width / 3.)) {
					if (NEAR(rel_y, 0)) {
						// top right col
						out.pos = POS_TOP_RIGHT;
						out.mode = MODE_THREE_COL_S;
					} else if (NEAR(rel_y, viewport.height / 2.)) {
						// bottom right col
						out.pos = POS_BOT_RIGHT;
						out.mode = MODE_THREE_COL_S;
					}
				}
			} else if (NEAR(window.height, viewport.height) &&
					NEAR(rel_y, 0)) {
				if (NEAR(rel_x, 0)) {
					// left col
					out.pos = POS_LEFT;
					out.mode = MODE_THREE_COL_S;
				} else if (NEAR(rel_x, viewport.width / 3.)) {
					// center col
					out.pos = POS_CENTER;
					out.mode = MODE_THREE_COL_S;
				} else if (NEAR(rel_x, 2 * viewport.width / 3.)) {
					// right col
					out.pos = POS_RIGHT;
					out.mode = MODE_THREE_COL_S;
				}
			}
		} else if (NEAR(window.width, 2 * viewport.width / 3.)) {
			if (NEAR(window.height, viewport.height / 2.)) {
				if (NEAR(rel_x, 0)) {
					if (NEAR(rel_y, 0)) {
						// top left two cols
						out.pos = POS_TOP_LEFT;
						out.mode = MODE_THREE_COL_L;
					} else if (NEAR(rel_y, viewport.height / 2.)) {
						// bottom left two cols
						out.pos = POS_BOT_LEFT;
						out.mode = MODE_THREE_COL_L;
					}
				} else if (NEAR(rel_x, viewport.width / 3.)) {
					if (NEAR(rel_y, 0)) {
						// top right two cols
						out.pos = POS_TOP_RIGHT;
						out.mode = MODE_THREE_COL_L;
					} else if (NEAR(rel_y, viewport.height / 2.)) {
						// bottom right two cols
						out.pos = POS_BOT_RIGHT;
						out.mode = MODE_THREE_COL_L;
					}
				}
			} else if (NEAR(window.height, viewport.height) &&
					NEAR(rel_y, 0)) {
				if (NEAR(rel_x, 0)) {
					// left two cols
					out.pos = POS_LEFT;
					out.mode = MODE_THREE_COL_L;
				} else if (NEAR(rel_x, viewport.width / 3.)) {
					// right two cols
					out.pos = POS_RIGHT;
					out.mode = MODE_THREE_COL_L;
				}
			}
		} else if (NEAR(window.width, viewport.width) &&
				NEAR(rel_x, 0)) {
			if (NEAR(window.height, viewport.height / 2.)) {
				if (NEAR(rel_y, 0)) {
					// top half
					out.pos = POS_TOP_CENTER;
					out.mode = MODE_THREE_COL_L;
				} else if (NEAR(rel_y, viewport.height / 2.)) {
					// bottom half
					out.pos = POS_BOT_CENTER;
					out.mode = MODE_THREE_COL_L;
				}
			} else if (NEAR(window.height, viewport.height) &&
					NEAR(rel_y, 0)) {
				// full screen
				out.pos = POS_CENTER;
				out.mode = MODE_THREE_COL_L;
			}
		}

		DEBUG("%ldx %ldy %luw %luh -> pos=%s mode=%s",
				rel_x, rel_y, window.width, window.height,
				pos_str(out.pos), mode_str(out.mode));
		return true;
	}

	bool get_next_pos(const Position& cur, POS req_pos, Position& out) {
		if (req_pos == POS_UNKNOWN) {
			return false;// nice to have
		}
		if (cur.pos == req_pos) {
			// position is same, so rotate mode
			out.pos = cur.pos;
			switch (cur.pos) {
			case POS_TOP_CENTER:
			case POS_CENTER:
			case POS_BOT_CENTER:
				// for center col: 3x2L -> 3x2S (-> 3x2L) (no 2x2)
				switch (cur.mode) {
				case MODE_THREE_COL_L:
					out.mode = MODE_THREE_COL_S;
					break;
				case MODE_THREE_COL_S:
				default:
					out.mode = MODE_THREE_COL_L;
				}
				break;
			default:
				// for everything else: 2x2 -> 3x2L -> 3x2S (-> 2x2)
				switch (cur.mode) {
				case MODE_UNKNOWN:
				case MODE_THREE_COL_L:
					out.mode = MODE_THREE_COL_S;
					break;
				case MODE_TWO_COL:
					out.mode = MODE_THREE_COL_L;
					break;
				case MODE_THREE_COL_S:
				default:
					out.mode = MODE_TWO_COL;
				}
			}
		} else {
			// new position, so start with initial mode
			out.pos = req_pos;
			switch (req_pos) {
			case POS_TOP_CENTER:
			case POS_CENTER:
			case POS_BOT_CENTER:
				// for center col: start with 3x2L
				out.mode = MODE_THREE_COL_L;
				break;
			default:
				// for everything else: start with 2x2
				out.mode = MODE_TWO_COL;
			}
		}
		DEBUG("curpos=%s curmode=%s + reqpos=%s -> pos=%s mode=%s",
				pos_str(cur.pos), mode_str(cur.mode), pos_str(req_pos),
				pos_str(out.pos), mode_str(out.mode));
		return true;
	}
}

bool grid::set_position(POS req_pos) {
	ActiveWindow win;
	Position cur_pos, next_pos;
	ActiveWindow::Dimensions viewport, cur_dim, next_dim;
	return win.Sizes(viewport, cur_dim) &&
		get_current_pos(cur_dim, viewport, cur_pos) &&// cur_dim+viewport -> cur_pos
		get_next_pos(cur_pos, req_pos, next_pos) &&// cur_pos+req_pos -> next_pos
		calculate_pos(viewport, next_pos, next_dim) &&// next_pos+viewport -> next_dim
		win.MoveResize(next_dim);
}
