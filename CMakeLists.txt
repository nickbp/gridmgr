cmake_minimum_required (VERSION 2.6)

project(gridmgr)

set (gridmgr_VERSION_MAJOR 1)
set (gridmgr_VERSION_MINOR 0)
set (gridmgr_VERSION_PATCH 0)

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

find_package(X11 REQUIRED)

include_directories("${PROJECT_BINARY_DIR}"
  "${X11_X11_INCLUDE_PATH}" "${X11_Xinerama_INCLUDE_PATH}")
add_executable(gridmgr ${SRCS})
target_link_libraries(gridmgr
  "${X11_X11_LIB}" "${X11_Xinerama_LIB}")

include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE
  "${CMAKE_CURRENT_SOURCE_DIR}/LICENCE.txt")
set (CPACK_PACKAGE_VERSION_MAJOR "${gridmgr_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${gridmgr_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${gridmgr_VERSION_PATCH}")
include (CPack)
