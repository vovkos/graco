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
	NAME graco-jancy
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/jancy
	COMMAND $<TARGET_FILE:graco> jnc_ct_Parser.llk
	)

add_test (
	NAME graco-java
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	COMMAND $<TARGET_FILE:graco> java.llk
	)

add_test (
	NAME graco-c
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	COMMAND $<TARGET_FILE:graco> c.llk
	)

add_test (
	NAME graco-left-recursion
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	COMMAND $<TARGET_FILE:graco> left-recursion.llk
	)

set_tests_properties (
	graco-left-recursion
	PROPERTIES
		WILL_FAIL TRUE
	)

#...............................................................................
