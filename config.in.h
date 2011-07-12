#ifndef GRID_CONFIG_H
#define GRID_CONFIG_H

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

#include <stdio.h>

namespace grid {
	namespace config {
		static const int
			VERSION_MAJOR = @grid_VERSION_MAJOR@,
			VERSION_MINOR = @grid_VERSION_MINOR@,
			VERSION_PATCH = @grid_VERSION_PATCH@;

		static const char* VERSION_STRING = "@grid_VERSION_MAJOR@.@grid_VERSION_MINOR@.@grid_VERSION_PATCH@";
		static const char* BUILD_DATE = __TIMESTAMP__;

		extern bool debug_enabled;
		extern FILE *fout;
		extern FILE *ferr;

		void debug(const char* format, ...);
		void debugnn(const char* format, ...);
		void log(const char* format, ...);
		void lognn(const char* format, ...);
		void error(const char* format, ...);
		void errornn(const char* format, ...);
	}
}

#endif
