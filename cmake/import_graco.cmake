#..............................................................................

axl_find_file (
	_CONFIG_CMAKE
	graco_config.cmake
	${GRACO_CMAKE_DIR}
	)

if (_CONFIG_CMAKE)
	include (${_CONFIG_CMAKE})

	message (STATUS "Graco paths:")
	message (STATUS "    Graco cmake files: ${GRACO_CMAKE_DIR}")
	message (STATUS "    Graco includes:    ${GRACO_INC_DIR}")
	message (STATUS "    Graco frames:      ${GRACO_FRAME_DIR}")
	message (STATUS "    Graco executable:  ${GRACO_EXE}")

	set (GRACO_FOUND TRUE)
else ()
	set (GRACO_FOUND FALSE)
endif ()

#..............................................................................
