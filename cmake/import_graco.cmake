#..............................................................................

axl_find_file (
	_CONFIG_CMAKE
	graco_config.cmake
	${GRACO_CMAKE_DIR}
	)

if (_CONFIG_CMAKE)
	include (${_CONFIG_CMAKE})

	message (STATUS "Path definitions for Graco:")
	axl_message ("    CMake files:" "${GRACO_CMAKE_DIR}")
	axl_message ("    Includes:"    "${GRACO_INC_DIR}")
	axl_message ("    Frames:"      "${GRACO_FRAME_DIR}")
	axl_message ("    Executable:"  "${GRACO_EXE}")

	set (GRACO_FOUND TRUE)
else ()
	set (GRACO_FOUND FALSE)
endif ()

#..............................................................................
