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
	BUILD_GRACO_SAMPLE_01_CALC
	"Build graco_sample_01_calc - calculator with variables"
	ON
)

if(BUILD_GRACO_SAMPLE_01_CALC)
	add_subdirectory(graco_sample_01_calc)
endif()

#...............................................................................
