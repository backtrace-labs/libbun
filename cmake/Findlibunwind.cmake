# - Try to find LIBUNWIND
# Once done this will define
#
# set CMake variable LIBUNWIND_DIR to point at your custom
# LIBUNWIND build.
#
#  LIBUNWIND_FOUND - system has LIBUNWIND
#  LIBUNWIND_INCLUDE_DIR - the LIBUNWIND include directory
#
#  Copyright (c) 2021 Backtrace I/O <team@backtrace.io>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (LIBUNWIND_LIBRARIES AND LIBUNWIND_INCLUDE_DIR)
  # in cache already
  set(LIBUNWIND_FOUND TRUE)
else (LIBUNWIND_LIBRARIES AND LIBUNWIND_INCLUDE_DIR)
  find_path(LIBUNWIND_INCLUDE_DIR
    NAMES
      libunwind.h
    PATHS
      ${LIBUNWIND_DIR}
      /usr/include/libunwind
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(LIBUNWIND_LIBRARIES NAMES unwind)

  if (LIBUNWIND_INCLUDE_DIR AND LIBUNWIND_LIBRARIES)
    set(LIBUNWIND_FOUND TRUE)
  endif()

  if (LIBUNWIND_FOUND)
    if (NOT LIBUNWIND_FIND_QUIETLY)
      message(STATUS "Found LIBUNWIND: ${LIBUNWIND_OBJECTS}")
    endif (NOT LIBUNWIND_FIND_QUIETLY)
  else (LIBUNWIND_FOUND)
    if (LIBUNWIND_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find LIBUNWIND")
    endif (LIBUNWIND_FIND_REQUIRED)
  endif (LIBUNWIND_FOUND)

  # show the LIBUNWIND_INCLUDE_DIR variables only in the advanced view
  mark_as_advanced(LIBUNWIND_INCLUDE_DIR LIBUNWIND_OBJECTS)

endif (LIBUNWIND_LIBRARIES AND LIBUNWIND_INCLUDE_DIR)
