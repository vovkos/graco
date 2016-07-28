#..............................................................................

set (GRACO_VERSION_MAJOR     1)
set (GRACO_VERSION_MINOR     2)
set (GRACO_VERSION_REVISION  1)
set (GRACO_VERSION_TAG)

set (GRACO_VERSION_FILE_SUFFIX "${GRACO_VERSION_MAJOR}.${GRACO_VERSION_MINOR}.${GRACO_VERSION_REVISION}")

if (NOT "${GRACO_VERSION_TAG}" STREQUAL "")
	set (GRACO_VERSION_TAG_SUFFIX " ${GRACO_VERSION_TAG}")
	set (GRACO_VERSION_FILE_SUFFIX "${GRACO_VERSION_FILE_SUFFIX}-${GRACO_VERSION_TAG}")
else ()
	set (GRACO_VERSION_TAG_SUFFIX)
endif ()

string (TIMESTAMP GRACO_VERSION_YEAR  "%Y")
string (TIMESTAMP GRACO_VERSION_MONTH "%m")
string (TIMESTAMP GRACO_VERSION_DAY   "%d")

set (GRACO_VERSION_COMPANY   "Tibbo Technology Inc")
set (GRACO_VERSION_YEARS     "2012-${GRACO_VERSION_YEAR}")

#..............................................................................
