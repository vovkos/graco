#...............................................................................
#
#  This file is part of the Graco toolkit.
#
#  Graco is distributed under the MIT license.
#  For details see accompanying license.txt file,
#  the public copy of which is also available at:
#  http://tibbo.com/downloads/archive/graco/license.txt
#
#...............................................................................

set (GRACO_ROOT_DIR   "${CMAKE_CURRENT_LIST_DIR}/..")
set (GRACO_CMAKE_DIR  "${CMAKE_CURRENT_LIST_DIR}")
set (GRACO_INC_DIR    "${GRACO_ROOT_DIR}/include")
set (GRACO_FRAME_DIR  "${GRACO_ROOT_DIR}/frame")
set (GRACO_SAMPLE_DIR "${GRACO_ROOT_DIR}/samples")
set (GRACO_EXE        "${GRACO_ROOT_DIR}/bin/graco")

include (version.cmake)
include (graco_step.cmake)

#...............................................................................
