# This file is part of AXL (R) Library
# Tibbo Technology Inc (C) 2004-2013. All rights reserved
# Author: Vladimir Gladkov

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
