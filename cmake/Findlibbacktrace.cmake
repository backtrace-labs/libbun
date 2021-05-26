# - Try to find LIBBACKTRACE
# Once done this will define
#
# set CMake variable LIBBACKTRACE_DIR to point at your custom
# LIBBACKTRACE build.
#
#  LIBBACKTRACE_FOUND - system has LIBBACKTRACE
#  LIBBACKTRACE_INCLUDE_DIR - the LIBBACKTRACE include directory
#
#  Copyright (c) 2021 Backtrace I/O <team@backtrace.io>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (LIBBACKTRACE_LIBRARIES AND LIBBACKTRACE_INCLUDE_DIR)
  # in cache already
  set(LIBBACKTRACE_FOUND TRUE)
else (LIBBACKTRACE_LIBRARIES AND LIBBACKTRACE_INCLUDE_DIR)
  find_path(LIBBACKTRACE_INCLUDE_DIR
    NAMES
      libbacktrace/backtrace.h
    PATHS
      ${LIBBACKTRACE_DIR}
      /usr/include/backtrace
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_library(LIBBACKTRACE_LIBRARIES NAMES backtrace )

  if (LIBBACKTRACE_FOUND)
    if (NOT LIBBACKTRACE_FIND_QUIETLY)
      message(STATUS "Found LIBBACKTRACE: ${LIBBACKTRACE_OBJECTS}")
    endif (NOT LIBBACKTRACE_FIND_QUIETLY)
  else (LIBBACKTRACE_FOUND)
    if (LIBBACKTRACE_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find LIBBACKTRACE")
    endif (LIBBACKTRACE_FIND_REQUIRED)
  endif (LIBBACKTRACE_FOUND)

  # show the LIBBACKTRACE_INCLUDE_DIR variables only in the advanced view
  mark_as_advanced(LIBBACKTRACE_INCLUDE_DIR LIBBACKTRACE_OBJECTS)

endif (LIBBACKTRACE_LIBRARIES AND LIBBACKTRACE_INCLUDE_DIR)
