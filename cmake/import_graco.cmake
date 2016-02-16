#..............................................................................

if ("${GRACO_CMAKE_DIR_2}" STREQUAL "")
	set (GRACO_FOUND FALSE)
	message (STATUS "Graco:                      <not-found>")
else ()
	include (${GRACO_CMAKE_DIR_2}/graco_config.cmake)

	set (GRACO_FOUND TRUE)
	message (STATUS "Path to Graco cmake files:  ${GRACO_CMAKE_DIR}")
	message (STATUS "Path to Graco includes:     ${GRACO_INC_DIR}")
	message (STATUS "Path to Graco frames:       ${GRACO_FRAME_DIR}")
	message (STATUS "Path to Graco executable:   ${GRACO_EXE}")
endif ()

#..............................................................................
