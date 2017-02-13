#...............................................................................
#
#  This file is part of the Jancy toolkit.
#
#  Jancy is distributed under the MIT license.
#  For details see accompanying license.txt file,
#  the public copy of which is also available at:
#  http://tibbo.com/downloads/archive/jancy/license.txt
#
#...............................................................................

add_test (
	NAME jancy
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/jancy
	COMMAND $<TARGET_FILE:graco> jnc_ct_Parser.llk
	)

add_test (
	NAME java
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	COMMAND $<TARGET_FILE:graco> java.llk
	)

add_test (
	NAME c
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	COMMAND $<TARGET_FILE:graco> c.llk
	)

add_test (
	NAME left-recursion
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	COMMAND $<TARGET_FILE:graco> left-recursion.llk
	)

set_tests_properties (
	left-recursion
	PROPERTIES
		WILL_FAIL TRUE
	)

#...............................................................................
