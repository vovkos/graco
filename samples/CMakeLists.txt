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

option(
	BUILD_GRACO_SAMPLES
	"Build Graco samples"
	ON
)

if(BUILD_GRACO_SAMPLES)
	add_subdirectory(graco_sample_01_calc)
endif()

#...............................................................................
