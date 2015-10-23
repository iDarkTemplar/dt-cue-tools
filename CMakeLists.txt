project(DT-Cue-Tools)

cmake_minimum_required( VERSION 2.8.8 )

set(DTCT_VERSION_MAJOR 0)
set(DTCT_VERSION_MINOR 1)
set(DTCT_VERSION_PATCH 4)
set(DTCT_VERSION "${DTCT_VERSION_MAJOR}.${DTCT_VERSION_MINOR}.${DTCT_VERSION_PATCH}")

set(CPACK_PACKAGE_VERSION_MAJOR ${DTCT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DTCT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DTCT_VERSION_PATCH})
set(CPACK_GENERATOR "TBZ2")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES
	".git;build;CMakeLists.txt.user;CMakeLists.txt.user.*;ignore;TODO;cppcheck-linux.sh")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${DTCT_VERSION}")
# MUST be after CPACK_* variables.
include(CPack)

# installation directory configuration
set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Installation prefix for executables and object code libraries" FORCE)
set(BIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/bin CACHE PATH "Installation prefix for user executables" FORCE)
set(SBIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/sbin CACHE PATH "Installation prefix for user executables" FORCE)
set(LIB_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX} CACHE PATH  "Installation prefix for object code libraries" FORCE)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include CACHE PATH "Installation prefix for C header files" FORCE)
set(DATA_PREFIX "${CMAKE_INSTALL_PREFIX}/share" CACHE PATH "Prefix prepended to data install directories" FORCE)

option(ENABLE_LIBVERSION "enable libraries versioning" ON)
option(ENABLE_SPLIT_TOOL "enable split tool" ON)

message(STATUS "System: ${CMAKE_SYSTEM_NAME}" )

# don't USE -O3 with GCC, it causes less precise calculations
if (CMAKE_COMPILER_IS_GNUCC)
	set (CMAKE_C_FLAGS_RELEASE "-O2 -pipe -march=native -Wall -Wextra -Wno-unused-result -DNDEBUG")
	set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

	set (CMAKE_C_FLAGS_DEBUG "-O0 -pipe -march=native -Wall -Wextra -Wno-unused-result -g -ggdb")
	set (CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
endif (CMAKE_COMPILER_IS_GNUCC)

# clang compiler options
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set (CLANG_WARNINGS "-Weverything -Wno-missing-prototypes -Wno-padded -Wno-sign-conversion -Wno-conversion -Wno-disabled-macro-expansion -Wno-cast-align" )

	set (CMAKE_C_FLAGS_RELEASE "-O3 -pipe ${CLANG_WARNINGS} -DNDEBUG")
	set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

	set (CMAKE_C_FLAGS_DEBUG "-O0 -pipe ${CLANG_WARNINGS} -g -ggdb")
	set (CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
endif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

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
set ( CUE_LIBRARY_HEADERS cue-library/dt-cue-library.hpp )

set ( CUE_APP_SOURCES cue-splitter/cue-splitter.cpp )
set ( CUE_APP_HEADERS )

add_library( dt-cue-parser SHARED ${CUE_LIBRARY_SOURCES} ${CUE_LIBRARY_HEADERS} )
if (ENABLE_LIBVERSION)
	set_target_properties( dt-cue-parser PROPERTIES VERSION ${DTCT_VERSION} SOVERSION ${DTCT_VERSION_MAJOR} )
endif (ENABLE_LIBVERSION)
target_link_libraries( dt-cue-parser ${Boost_LIBRARIES} )

if (ENABLE_SPLIT_TOOL)
	add_executable( dt-cue-split ${CUE_APP_SOURCES} ${CUE_APP_HEADERS})
	target_link_libraries( dt-cue-split dt-cue-parser ${Boost_LIBRARIES} )
endif (ENABLE_SPLIT_TOOL)

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
install(TARGETS dt-cue-parser LIBRARY DESTINATION ${LIB_INSTALL_DIR} )

if (ENABLE_SPLIT_TOOL)
	install(TARGETS dt-cue-split RUNTIME DESTINATION ${BIN_INSTALL_DIR})
endif (ENABLE_SPLIT_TOOL)

install(FILES ${CUE_LIBRARY_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}")
