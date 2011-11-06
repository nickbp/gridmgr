cmake_minimum_required (VERSION 2.6)

project(gridmgr)

find_package(X11 REQUIRED)

if (X11_Xinerama_INCLUDE_PATH AND X11_Xinerama_LIB)
  message(STATUS "Found Xinerama: ${X11_Xinerama_LIB}")
  set(FOUND_XINERAMA ON)
else()
  message(STATUS "Didn't find Xinerama.")
endif()

option(USE_XINERAMA "Enable Xinerama multi-monitor support" ${FOUND_XINERAMA})

set (gridmgr_VERSION_MAJOR 1)
set (gridmgr_VERSION_MINOR 0)
set (gridmgr_VERSION_PATCH 0)

# Enable c++0x and more warnings
if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-std=c++0x -Wall")
endif()

SET(SRCS
  config.in.h
  config.cpp
  grid.h
  grid.cpp
  main.cpp
  position.h
  position.cpp
  viewport.h
  viewport-ewmh.cpp
  window.h
  window.cpp
  x11-util.h
  x11-util.cpp
  )
configure_file (
  "${PROJECT_SOURCE_DIR}/config.in.h"
  "${PROJECT_BINARY_DIR}/config.h"
  )

set(INCLUDES
  "${X11_X11_INCLUDE_PATH}"
  )
set(LIBS
  "${X11_X11_LIB}"
  )

if(USE_XINERAMA)

  message(STATUS "Xinerama multi-monitor support enabled.")
  list(APPEND INCLUDES "${X11_Xinerama_INCLUDE_PATH}")
  list(APPEND LIBS "${X11_Xinerama_LIB}")
  list(APPEND SRCS viewport-xinerama.cpp)

else()

  message(STATUS "Xinerama multi-monitor support disabled.")

endif()

include_directories("${PROJECT_BINARY_DIR}" ${INCLUDES})
add_executable(gridmgr ${SRCS})
target_link_libraries(gridmgr ${LIBS})

include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE
  "${CMAKE_CURRENT_SOURCE_DIR}/../LICENCE")
set (CPACK_PACKAGE_VERSION_MAJOR "${gridmgr_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${gridmgr_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${gridmgr_VERSION_PATCH}")
include (CPack)
