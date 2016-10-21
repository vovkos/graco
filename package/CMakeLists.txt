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

if (WIN32)
	add_subdirectory (windows)
elseif (UNIX)
	add_subdirectory (unix)
endif ()

#...............................................................................
