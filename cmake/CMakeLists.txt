# This file is part of Bulldozer project
# Tibbo Technology Inc (C) 2004-2013. All rights reserved
# Author: Vladimir Gladkov

#..............................................................................

configure_file (
	bulldozer.cmake.in
	bulldozer.cmake
	@ONLY
	)

configure_file (
	bulldozer.cmake.in
	${CMAKE_CURRENT_LIST_DIR}/latest/bulldozer-${TARGET_CPU}.cmake
	@ONLY
	)

configure_file (
	bulldozer-latest.cmake.in
	${CMAKE_CURRENT_LIST_DIR}/latest/bulldozer.cmake
	@ONLY
	)

#..............................................................................
