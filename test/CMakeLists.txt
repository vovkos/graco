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
	BUILD_TEST_CPP
	"Build test_cpp - test for C++ back-end"
	ON
	)

if (BUILD_TEST_CPP)
	add_subdirectory (test_cpp)
endif ()

#...............................................................................
