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

cmake_minimum_required(VERSION 3.16)

project(graco)

include(cmake/axl_cmake.cmake NO_POLICY_SCOPE)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

set(GRACO_ROOT_DIR     ${CMAKE_CURRENT_LIST_DIR})
set(GRACO_BIN_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(GRACO_DOC_DIR      ${CMAKE_CURRENT_BINARY_DIR}/doc)
set(GRACO_PACKAGE_DIR  ${CMAKE_CURRENT_BINARY_DIR}/package)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GRACO_BIN_BASE_DIR}/${CONFIGURATION_SCG})

set(GRACO_INSTALL_BIN_SUBDIR bin)

if(WIN32)
	set(GRACO_INSTALL_INC_SUBDIR     include)
	set(GRACO_INSTALL_CMAKE_SUBDIR   cmake)
	set(GRACO_INSTALL_FRAME_SUBDIR   frame)
	set(GRACO_INSTALL_SAMPLE_SUBDIR  samples)
	set(GRACO_INSTALL_DOC_SUBDIR     doc)
	set(GRACO_INSTALL_LICENSE_SUBDIR license)
else()
	set(GRACO_INSTALL_INC_SUBDIR     include/${PROJECT_NAME})
	set(GRACO_INSTALL_CMAKE_SUBDIR   lib/cmake/${PROJECT_NAME})
	set(GRACO_INSTALL_FRAME_SUBDIR   share/${PROJECT_NAME}/frame)
	set(GRACO_INSTALL_SAMPLE_SUBDIR  share/${PROJECT_NAME}/samples)
	set(GRACO_INSTALL_DOC_SUBDIR     share/${PROJECT_NAME}/doc)
	set(GRACO_INSTALL_LICENSE_SUBDIR share/${PROJECT_NAME}/license)
endif()

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

add_subdirectory(cmake)

include(${CMAKE_CURRENT_BINARY_DIR}/cmake/graco_config.cmake)

add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(samples)
add_subdirectory(test)

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

install(
	DIRECTORY include/
	DESTINATION ${GRACO_INSTALL_INC_SUBDIR}
)

install(
	DIRECTORY frame/
	DESTINATION ${GRACO_INSTALL_FRAME_SUBDIR}
)

install(
	DIRECTORY samples/
	DESTINATION ${GRACO_INSTALL_SAMPLE_SUBDIR}
)

install(
	DIRECTORY license/
	DESTINATION ${GRACO_INSTALL_LICENSE_SUBDIR}
)

#...............................................................................

set(CPACK_PACKAGE_VERSION_MAJOR ${GRACO_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GRACO_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GRACO_VERSION_REVISION})

if(WIN32)
	set(CPACK_SOURCE_GENERATOR 7Z)
else()
	set(CPACK_SOURCE_GENERATOR TXZ)
endif()

set(CPACK_SOURCE_IGNORE_FILES "/[.];[.]user$;/build/;/ci/;/paths.cmake$")

set(CPACK_SOURCE_PACKAGE_FILE_NAME ${PROJECT_NAME}-${GRACO_VERSION_FILE_SUFFIX}-src)

if(APPLE)
	set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${GRACO_VERSION_FILE_SUFFIX}-mac)
else()
	string(TOLOWER ${CMAKE_SYSTEM_NAME} OS_NAME)
	set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${GRACO_VERSION_FILE_SUFFIX}-${OS_NAME}-${TARGET_CPU})
endif()

include(CPack)

#...............................................................................
