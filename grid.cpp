/*
  grid - Organizes windows according to a grid.
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
		MODE_TWO_COL,//2x2 (not applicable for center col positions)
		MODE_THREE_COL_S,//3x2 small: each position filling 1 column
		MODE_THREE_COL_L//3x2 large: sides filling 2 columns and center filling full width
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
	bool calculate_pos(const ActiveWindow::Size& viewport, const Position& pos,
			ActiveWindow::Dimensions& window_out) {
		bool ret = true;
		switch (pos.mode) {
		case MODE_TWO_COL:
			switch (pos.pos) {
			case POS_TOP_LEFT://top left quadrant
				window_out.x = 0;
				window_out.y = 0;
				window_out.width = viewport.width / 2.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_TOP_CENTER://invalid, use mode THREE_COL_S/L
				return false;
			case POS_TOP_RIGHT://top right quadrant
				window_out.x = viewport.width / 2.;
				window_out.y = 0;
				window_out.width = viewport.width / 2.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_LEFT://left half
				window_out.x = 0;
				window_out.y = 0;
				window_out.width = viewport.width / 2.;
				window_out.height = viewport.height;
				break;
			case POS_CENTER://invalid, use mode THREE_COL_S/L
				return false;
			case POS_RIGHT://right half
				window_out.x = viewport.width / 2.;
				window_out.y = 0;
				window_out.width = viewport.width / 2.;
				window_out.height = viewport.height;
				break;
			case POS_BOT_LEFT://bottom left quadrant
				window_out.x = 0;
				window_out.y = viewport.height / 2.;
				window_out.width = viewport.width / 2.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_BOT_CENTER://invalid, use mode THREE_COL_S/L
				return false;
			case POS_BOT_RIGHT://bottom right quadrant
				window_out.x = viewport.width / 2.;
				window_out.y = viewport.height / 2.;
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
			case POS_TOP_LEFT://top left col
				window_out.x = 0;
				window_out.y = 0;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_TOP_CENTER://top center col
				window_out.x = viewport.width / 3.;
				window_out.y = 0;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_TOP_RIGHT://top right col
				window_out.x = 2 * viewport.width / 3.;
				window_out.y = 0;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_LEFT://left third
				window_out.x = 0;
				window_out.y = 0;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height;
				break;
			case POS_CENTER://center col
				window_out.x = viewport.width / 3.;
				window_out.y = 0;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height;
				break;
			case POS_RIGHT://right third
				window_out.x = 2 * viewport.width / 3.;
				window_out.y = 0;
				window_out.width = viewport.width / 3;
				window_out.height = viewport.height;
				break;
			case POS_BOT_LEFT://bottom left col
				window_out.x = 0;
				window_out.y = viewport.height / 2.;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_BOT_CENTER://bottom center col
				window_out.x = viewport.width / 3.;
				window_out.y = viewport.height / 2.;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_BOT_RIGHT://bottom right col
				window_out.x = 2 * viewport.width / 3.;
				window_out.y = viewport.height / 2.;
				window_out.width = viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			default:
				ret = false;
			}
			break;
		case MODE_THREE_COL_L:
			switch (pos.pos) {
			case POS_TOP_LEFT://top left two cols
				window_out.x = 0;
				window_out.y = 0;
				window_out.width = 2 * viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_TOP_CENTER://top half
				window_out.x = 0;
				window_out.y = 0;
				window_out.width = viewport.width;
				window_out.height = viewport.height / 2.;
				break;
			case POS_TOP_RIGHT://top right two cols
				window_out.x = viewport.width / 3.;
				window_out.y = 0;
				window_out.width = 2 * viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_LEFT://left two thirds
				window_out.x = 0;
				window_out.y = 0;
				window_out.width = 2 * viewport.width / 3.;
				window_out.height = viewport.height;
				break;
			case POS_CENTER://full screen
				window_out.x = 0;
				window_out.y = 0;
				window_out.width = viewport.width;
				window_out.height = viewport.height;
				break;
			case POS_RIGHT://right two thirds
				window_out.x = viewport.width / 3.;
				window_out.y = 0;
				window_out.width = 2 * viewport.width / 3.;
				window_out.height = viewport.height;
				break;
			case POS_BOT_LEFT://bottom left two cols
				window_out.x = 0;
				window_out.y = viewport.height / 2.;
				window_out.width = 2 * viewport.width / 3.;
				window_out.height = viewport.height / 2.;
				break;
			case POS_BOT_CENTER://bottom half
				window_out.x = 0;
				window_out.y = viewport.height / 2.;
				window_out.width = viewport.width;
				window_out.height = viewport.height / 2.;
				break;
			case POS_BOT_RIGHT://bottom right two cols
				window_out.x = viewport.width / 3.;
				window_out.y = viewport.height / 2.;
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
			config::debug("calculate_pos pos=%s mode=%s -> %ldx %ldy %luw %luh",
					pos_str(pos.pos), mode_str(pos.mode),
					window_out.x, window_out.y,
					window_out.width, window_out.height);
		} else {
			config::error("bad pos %s + mode %s", pos_str(pos.pos), mode_str(pos.mode));
		}
		return ret;
	}

	// whether the two numbers are within some threshold of one another,
	// where the threshold is calculated from the largest number
	// (eg 990 is 'near' 1000 for a threshold of 1%)
	template <typename A, typename B>
	inline bool near(A a, B b) {
		double offset = 0.05 * ((a > b) ? a : b);
		return (a + offset) >= b && (a - offset) <= b;
	}

	//given window's dimensions, estimate its position/mode (or unknown+unknown)
	//(inverse of calculate_pos)
	bool get_current_pos(const ActiveWindow::Dimensions& dim,
			const ActiveWindow::Size& viewport, Position& out) {
		out.pos = POS_UNKNOWN;
		out.mode = MODE_UNKNOWN;
		if (near(dim.width, viewport.width / 2.)) {
			if (near(dim.height, viewport.height / 2.)) {
				if (near(dim.x, 0)) {
					if (near(dim.y, 0)) {
						//top left quadrant
						out.pos = POS_TOP_LEFT;
						out.mode = MODE_TWO_COL;
					} else if (near(dim.y, viewport.height / 2.)) {
						//bottom left quadrant
						out.pos = POS_BOT_LEFT;
						out.mode = MODE_TWO_COL;
					}
				} else if (near(dim.x, viewport.width / 2.)) {
					if (near(dim.y, 0)) {
						//top right quadrant
						out.pos = POS_TOP_RIGHT;
						out.mode = MODE_TWO_COL;
					} else if (near(dim.y, viewport.height / 2.)) {
						//bottom right quadrant
						out.pos = POS_BOT_RIGHT;
						out.mode = MODE_TWO_COL;
					}
				}
			} else if (near(dim.height, viewport.height) &&
					near(dim.y, 0)) {
				if (near(dim.x, 0)) {
					//left half
					out.pos = POS_LEFT;
					out.mode = MODE_TWO_COL;
				} else if (near(dim.x, viewport.width / 2.)) {
					//right half
					out.pos = POS_RIGHT;
					out.mode = MODE_TWO_COL;
				}
			}
		} else if (near(dim.width, viewport.width / 3.)) {
			if (near(dim.height, viewport.height / 2.)) {
				if (near(dim.x, 0)) {
					if (near(dim.y, 0)) {
						//top left col
						out.pos = POS_TOP_LEFT;
						out.mode = MODE_THREE_COL_S;
					} if (near(dim.y, viewport.height / 2.)) {
						//bottom left col
						out.pos = POS_BOT_LEFT;
						out.mode = MODE_THREE_COL_S;
					}
				} else if (near(dim.x, viewport.width / 3.)) {
					if (near(dim.y, 0)) {
						//top center col
						out.pos = POS_TOP_CENTER;
						out.mode = MODE_THREE_COL_S;
					} else if (near(dim.y, viewport.height / 2.)) {
						//bottom center col
						out.pos = POS_BOT_CENTER;
						out.mode = MODE_THREE_COL_S;
					}
				} else if (near(dim.x, 2 * viewport.width / 3.)) {
					if (near(dim.y, 0)) {
						//top right col
						out.pos = POS_TOP_RIGHT;
						out.mode = MODE_THREE_COL_S;
					} else if (near(dim.y, viewport.height / 2.)) {
						//bottom right col
						out.pos = POS_BOT_RIGHT;
						out.mode = MODE_THREE_COL_S;
					}
				}
			} else if (near(dim.height, viewport.height) &&
					near(dim.y, 0)) {
				if (near(dim.x, 0)) {
					//left col
					out.pos = POS_LEFT;
					out.mode = MODE_THREE_COL_S;
				} else if (near(dim.x, viewport.width / 3.)) {
					//center col
					out.pos = POS_CENTER;
					out.mode = MODE_THREE_COL_S;
				} else if (near(dim.x, 2 * viewport.width / 3.)) {
					//right col
					out.pos = POS_RIGHT;
					out.mode = MODE_THREE_COL_S;
				}
			}
		} else if (near(dim.width, 2 * viewport.width / 3.)) {
			if (near(dim.height, viewport.height / 2.)) {
				if (near(dim.x, 0)) {
					if (near(dim.y, 0)) {
						//top left two cols
						out.pos = POS_TOP_LEFT;
						out.mode = MODE_THREE_COL_L;
					} else if (near(dim.y, viewport.height / 2.)) {
						//bottom left two cols
						out.pos = POS_BOT_LEFT;
						out.mode = MODE_THREE_COL_L;
					}
				} else if (near(dim.x, viewport.width / 3.)) {
					if (near(dim.y, 0)) {
						//top right two cols
						out.pos = POS_TOP_RIGHT;
						out.mode = MODE_THREE_COL_L;
					} else if (near(dim.y, viewport.height / 2.)) {
						//bottom right two cols
						out.pos = POS_BOT_RIGHT;
						out.mode = MODE_THREE_COL_L;
					}
				}
			} else if (near(dim.height, viewport.height) &&
					near(dim.y, 0)) {
				if (near(dim.x, 0)) {
					//left two cols
					out.pos = POS_LEFT;
					out.mode = MODE_THREE_COL_L;
				} else if (near(dim.x, viewport.width / 3.)) {
					//right two cols
					out.pos = POS_RIGHT;
					out.mode = MODE_THREE_COL_L;
				}
			}
		} else if (near(dim.width, viewport.width) &&
				near(dim.x, 0)) {
			if (near(dim.height, viewport.height / 2.)) {
				if (near(dim.y, 0)) {
					//top half
					out.pos = POS_TOP_CENTER;
					out.mode = MODE_THREE_COL_L;
				} else if (near(dim.y, viewport.height / 2.)) {
					//bottom half
					out.pos = POS_BOT_CENTER;
					out.mode = MODE_THREE_COL_L;
				}
			} else if (near(dim.height, viewport.height) &&
					near(dim.y, 0)) {
				//full screen
				out.pos = POS_CENTER;
				out.mode = MODE_THREE_COL_L;
			}
		}

		config::debug("get_current_pos %ldx %ldy %luw %luh -> pos=%s mode=%s",
				dim.x, dim.y, dim.width, dim.height,
				pos_str(out.pos), mode_str(out.mode));
		return true;
	}

	bool get_next_pos(const Position& cur, POS req_pos, Position& out) {
		if (req_pos == POS_UNKNOWN) {
			return false;//nice to have
		}
		if (cur.pos == req_pos) {
			//position is same, so rotate mode
			out.pos = cur.pos;
			switch (cur.pos) {
			case POS_TOP_CENTER:
			case POS_CENTER:
			case POS_BOT_CENTER:
				//for center col: 3x2L -> 3x2S (-> 3x2L) (no 2x2)
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
				//for everything else: 2x2 -> 3x2L -> 3x2S (-> 2x2)
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
			//new position, so start with initial mode
			out.pos = req_pos;
			switch (req_pos) {
			case POS_TOP_CENTER:
			case POS_CENTER:
			case POS_BOT_CENTER:
				//for center col: start with 3x2L
				out.mode = MODE_THREE_COL_L;
				break;
			default:
				//for everything else: start with 2x2
				out.mode = MODE_TWO_COL;
			}
		}
		config::debug("get_next_pos curpos=%s curmode=%s + reqpos=%s -> pos=%s mode=%s",
				pos_str(cur.pos), mode_str(cur.mode), pos_str(req_pos),
				pos_str(out.pos), mode_str(out.mode));
		return true;
	}
}

bool grid::set_position(POS req_pos) {
	ActiveWindow win;
	Position cur_pos, next_pos;
	ActiveWindow::Size viewport;
	ActiveWindow::Dimensions cur_dim, next_dim;
	return win.Sizes(viewport, cur_dim) &&
		get_current_pos(cur_dim, viewport, cur_pos) &&//cur_dim+viewport -> cur_pos
		get_next_pos(cur_pos, req_pos, next_pos) &&//cur_pos+req_pos -> next_pos
		calculate_pos(viewport, next_pos, next_dim) &&//next_pos+viewport -> next_dim
		win.MoveResize(next_dim);
}
