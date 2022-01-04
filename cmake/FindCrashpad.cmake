# - Try to find CRASHPAD
# Once done this will define
#
# set CMake variable CRASHPAD_DIR to point at your custom
# crashpad build.
#
#  CRASHPAD_FOUND - system has CRASHPAD
#  CRASHPAD_INCLUDE_DIRS - the CRASHPAD include directory
#  CRASHPAD_OBJECTS - Link these to use CRASHPAD
#  CRASHPAD_DEFINITIONS - Compiler switches required for using CRASHPAD
#
#  Copyright (c) 2021 Backtrace I/O <team@backtrace.io>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (CRASHPAD_OBJECTS AND CRASHPAD_INCLUDE_DIRS)
  # in cache already
  set(CRASHPAD_FOUND TRUE)
else (CRASHPAD_OBJECTS AND CRASHPAD_INCLUDE_DIRS)
  find_path(CRASHPAD_INCLUDE_DIR
    NAMES
      client/crashpad_client.h
    PATHS
      ${CRASHPAD_DIR}
      /usr/include/crashpad
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  find_path(CRASHPAD_OBJECT
    NAMES
      obj/third_party/mini_chromium/mini_chromium/base/base.rand_util.o
    PATHS
      ${CRASHPAD_INCLUDE_DIR}/out/Default
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (CRASHPAD_OBJECT)
    set(CRASHPAD_FOUND TRUE)
  endif (CRASHPAD_OBJECT)

  set(CRASHPAD_INCLUDE_DIRS
    ${CRASHPAD_INCLUDE_DIR}
    ${CRASHPAD_INCLUDE_DIR}/third_party/mini_chromium/mini_chromium/
  )

  if (CRASHPAD_FOUND)
    set(CRASHPAD_OBJECTS
      "${CRASHPAD_OBJECT}/obj/third_party/mini_chromium/mini_chromium/base/base.rand_util.o"
      "${CRASHPAD_OBJECT}/obj/third_party/mini_chromium/mini_chromium/base/memory/base.page_size_posix.o"
      "${CRASHPAD_OBJECT}/obj/third_party/mini_chromium/mini_chromium/base/strings/base.string_number_conversions.o"
      "${CRASHPAD_OBJECT}/obj/third_party/mini_chromium/mini_chromium/base/strings/base.utf_string_conversions.o"
      "${CRASHPAD_OBJECT}/obj/third_party/mini_chromium/mini_chromium/base/strings/base.utf_string_conversion_utils.o"
    )
    set(CRASHPAD_LIBRARIES common client base util compat pthread dl z)
    set(CRASHPAD_LIBRARY_DIRECTORIES
      "${CRASHPAD_OBJECT}/obj/client"
      "${CRASHPAD_OBJECT}/obj/third_party/mini_chromium/mini_chromium/base/"
      "${CRASHPAD_OBJECT}/obj/util"
      "${CRASHPAD_OBJECT}/obj/compat"
    )
  endif (CRASHPAD_FOUND)

  if (CRASHPAD_INCLUDE_DIRS AND CRASHPAD_OBJECTS)
     set(CRASHPAD_FOUND TRUE)
  endif (CRASHPAD_INCLUDE_DIRS AND CRASHPAD_OBJECTS)

  if (CRASHPAD_FOUND)
    if (NOT CRASHPAD_FIND_QUIETLY)
      message(STATUS "Found CRASHPAD: ${CRASHPAD_OBJECTS}")
    endif (NOT CRASHPAD_FIND_QUIETLY)
  else (CRASHPAD_FOUND)
    if (CRASHPAD_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find CRASHPAD")
    endif (CRASHPAD_FIND_REQUIRED)
  endif (CRASHPAD_FOUND)

  # show the CRASHPAD_INCLUDE_DIRS and CRASHPAD_OBJECTS variables only in the advanced view
  mark_as_advanced(CRASHPAD_INCLUDE_DIRS CRASHPAD_OBJECTS)

endif (CRASHPAD_OBJECTS AND CRASHPAD_INCLUDE_DIRS)
