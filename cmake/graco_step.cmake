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

macro(
add_graco_step
	_OUTPUT_FILE_NAME
	_FRAME_FILE_NAME
	_INPUT_FILE_NAME
	# ...
	)

	set(_INPUT_PATH  "${CMAKE_CURRENT_SOURCE_DIR}/${_INPUT_FILE_NAME}")
	set(_FRAME_PATH  "${GRACO_FRAME_DIR}/${_FRAME_FILE_NAME}")
	set(_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/${_OUTPUT_FILE_NAME}")
	set(_DEPENDENCY_LIST ${ARGN})

	if(TARGET graco)
		list(APPEND _DEPENDENCY_LIST graco)
	endif()

	add_custom_command(
		OUTPUT ${_OUTPUT_PATH}
		MAIN_DEPENDENCY ${_INPUT_PATH}
		COMMAND ${GRACO_EXE}
			${_INPUT_PATH}
			-o${_OUTPUT_PATH}
			-f${_FRAME_PATH}
		DEPENDS ${_DEPENDENCY_LIST}
		)
endmacro()

macro(
add_graco_double_step
	_OUTPUT_FILE_NAME_1
	_OUTPUT_FILE_NAME_2
	_FRAME_FILE_NAME_1
	_FRAME_FILE_NAME_2
	_INPUT_FILE_NAME
	# ...
	)

	set(_INPUT_PATH    "${CMAKE_CURRENT_SOURCE_DIR}/${_INPUT_FILE_NAME}")
	set(_FRAME_PATH_1  "${GRACO_FRAME_DIR}/${_FRAME_FILE_NAME_1}")
	set(_FRAME_PATH_2  "${GRACO_FRAME_DIR}/${_FRAME_FILE_NAME_2}")
	set(_OUTPUT_PATH_1 "${CMAKE_CURRENT_BINARY_DIR}/${_OUTPUT_FILE_NAME_1}")
	set(_OUTPUT_PATH_2 "${CMAKE_CURRENT_BINARY_DIR}/${_OUTPUT_FILE_NAME_2}")
	set(_DEPENDENCY_LIST ${ARGN})

	if(TARGET graco)
		list(APPEND _DEPENDENCY_LIST graco)
	endif()

	add_custom_command(
		OUTPUT ${_OUTPUT_PATH_1}
		MAIN_DEPENDENCY ${_INPUT_PATH}
		COMMAND ${GRACO_EXE}
			${_INPUT_PATH}
			-o${_OUTPUT_PATH_1}
			-o${_OUTPUT_PATH_2}
			-f${_FRAME_PATH_1}
			-f${_FRAME_PATH_2}
		DEPENDS ${_DEPENDENCY_LIST}
		)
endmacro()

#...............................................................................
