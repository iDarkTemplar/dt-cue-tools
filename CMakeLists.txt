project(DT-Cue-Tools)

cmake_minimum_required( VERSION 2.8.8 )

set(DTCT_VERSION_MAJOR 0)
set(DTCT_VERSION_MINOR 1)
set(DTCT_VERSION_PATCH 0)
set(DTCT_VERSION "${DTCT_VERSION_MAJOR}.${DTCT_VERSION_MINOR}.${DTCT_VERSION_PATCH}")

set(CPACK_PACKAGE_VERSION_MAJOR ${DTCT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DTCT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DTCT_VERSION_PATCH})
set(CPACK_GENERATOR "TBZ2")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES
	".git;build;CMakeLists.txt.user;CMakeLists.txt.user.*;ignore;TODO")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${DTCT_VERSION}")
# MUST be after CPACK_* variables.
include(CPack)

# installation directory configuration
set(INST_PREFIX "/usr" CACHE PATH "Prefix prepended to install directories")
set(HEADERS_PREFIX "${INST_PREFIX}/include" CACHE PATH "Prefix prepended to headers install directories")
set(CMAKE_INSTALL_PREFIX "${INST_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

message(STATUS "To change installation directory use -DINST_PREFIX=path, for example, -DINST_PREFIX=/usr")

message(STATUS "System: ${CMAKE_SYSTEM_NAME}" )
if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	message(STATUS "OS: 64 bit")
	set (INSTALL_DIR_LIB "lib64")
else (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	message(STATUS "OS: 32 bit")
	set (INSTALL_DIR_LIB "lib32")
endif (${CMAKE_SIZEOF_VOID_P} EQUAL 8)

# don't USE -O3 with GCC, it causes less precise calculations
if (CMAKE_COMPILER_IS_GNUCC)
	set (CMAKE_C_FLAGS_RELEASE "-O2 -pipe -march=native -Wall -Wextra -Wno-unused-result -DNDEBUG")
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -std=c++0x")

	set (CMAKE_C_FLAGS_DEBUG "-O0 -pipe -march=native -Wall -Wextra -Wno-unused-result -g -ggdb")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -std=c++0x")
endif (CMAKE_COMPILER_IS_GNUCC)

# clang compiler options
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set (CLANG_WARNINGS "-Weverything -Wno-missing-prototypes -Wno-padded -Wno-sign-conversion -Wno-conversion -Wno-disabled-macro-expansion -Wno-cast-align" )

	set (CMAKE_C_FLAGS_RELEASE "-O3 -pipe ${CLANG_WARNINGS} -DNDEBUG")
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -std=c++0x")

	set (CMAKE_C_FLAGS_DEBUG "-O0 -pipe ${CLANG_WARNINGS} -g -ggdb")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -std=c++0x")
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

if (UNIX)
	set( CMAKE_THREAD_PREFER_PTHREAD 1 )
endif (UNIX)
find_package (Threads REQUIRED)

FIND_PACKAGE( Boost COMPONENTS regex REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

add_definitions(-D_FILE_OFFSET_BITS=64)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/cue-library )

set ( CUE_LIBRARY_SOURCES cue-library/dt-cue-library.cpp )
set ( CUE_LIBRARY_HEADERS cue-library/dt-cue-library.cpp )

set ( FLAC_APP_SOURCES flac-splitter/flac-splitter.cpp )
set ( FLAC_APP_HEADERS )

add_library( dt-cue-parser SHARED ${CUE_LIBRARY_SOURCES} ${CUE_LIBRARY_HEADERS} )
target_link_libraries( dt-cue-parser ${Boost_LIBRARIES} )

add_executable( dt-flac-split ${FLAC_APP_SOURCES} ${FLAC_APP_HEADERS})
target_link_libraries( dt-flac-split dt-cue-parser )

#ENABLE_TESTING()

#set (TEST_SOURCES_commands tests/commands_test.c tests/dt_tests.h)
#set (TEST_LIBS_commands dtmd-misc)

#set (ALL_TESTS commands decode_label filesystem_opts)
#foreach (CURRENT_TEST ${ALL_TESTS})
#	add_executable( ${CURRENT_TEST}_test ${TEST_SOURCES_${CURRENT_TEST}})
#	target_link_libraries( ${CURRENT_TEST}_test ${TEST_LIBS_${CURRENT_TEST}} )
#	add_test( ${CURRENT_TEST}_test ${CMAKE_CURRENT_BINARY_DIR}/${CURRENT_TEST}_test )
#endforeach (CURRENT_TEST)

# installation config
install(TARGETS dt-cue-parser LIBRARY DESTINATION ${INSTALL_DIR_LIB} )
install(TARGETS dt-flac-split RUNTIME DESTINATION bin)

install(FILES ${CUE_LIBRARY_HEADERS} DESTINATION "${HEADERS_PREFIX}")
