cmake_minimum_required (VERSION 2.6)

project(grid)

set (grid_VERSION_MAJOR 1)
set (grid_VERSION_MINOR 0)
set (grid_VERSION_PATCH 0)

SET(SRCS
  config.in.h
  config.cpp
  grid.h
  grid.cpp
  main.cpp
  window.h
  window.cpp
  )

configure_file (
  "${PROJECT_SOURCE_DIR}/config.in.h"
  "${PROJECT_BINARY_DIR}/config.h"
  )
include_directories("${PROJECT_BINARY_DIR}")

add_executable(grid ${SRCS})

target_link_libraries(grid
  X11
  )

include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE
  "${CMAKE_CURRENT_SOURCE_DIR}/LICENCE.txt")
set (CPACK_PACKAGE_VERSION_MAJOR "${grid_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${grid_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${grid_VERSION_PATCH}")
include (CPack)
