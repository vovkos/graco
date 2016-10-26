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

option (
	BUILD_GRACO_TEST_CPP
	"Build graco_test_cpp - test for C++ back-end"
	ON
	)

if (BUILD_GRACO_TEST_CPP)
	add_subdirectory (graco_test_cpp)
endif ()

#...............................................................................
