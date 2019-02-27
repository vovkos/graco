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

axl_find_file(
	_CONFIG_CMAKE
	graco_config.cmake
	${GRACO_CMAKE_DIR}
	)

if(_CONFIG_CMAKE)
	include(${_CONFIG_CMAKE})

	axl_message("Graco ${GRACO_VERSION_FULL} paths:")
	axl_message("    CMake files:" "${GRACO_CMAKE_DIR}")
	axl_message("    Includes:"    "${GRACO_INC_DIR}")
	axl_message("    Frames:"      "${GRACO_FRAME_DIR}")
	axl_message("    Executable:"  "${GRACO_EXE}")

	set(GRACO_FOUND TRUE)
else()
	set(GRACO_FOUND FALSE)
endif()

#...............................................................................
