cmake_minimum_required( VERSION 3.12.0 )

project(DT-Cue-Tools
	VERSION 0.4.1
	LANGUAGES CXX)

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

set (CMAKE_CXX_STANDARD 14)

add_definitions(-D_FILE_OFFSET_BITS=64)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/cue-library )

set ( CUE_LIBRARY_SOURCES cue-library/dt-cue-library.cpp )
set ( CUE_LIBRARY_HEADERS cue-library/dt-cue-library.hpp )

set ( CUE_APP_SOURCES cue-splitter/cue-splitter.cpp cue-splitter/cue-action.cpp)
set ( CUE_APP_HEADERS                               cue-splitter/cue-action.hpp)

add_library( dt-cue-parser SHARED ${CUE_LIBRARY_SOURCES} ${CUE_LIBRARY_HEADERS} )
if (ENABLE_LIBVERSION)
	set_target_properties( dt-cue-parser PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )
endif (ENABLE_LIBVERSION)
target_link_libraries( dt-cue-parser )

if (ENABLE_SPLIT_TOOL)
	add_executable( dt-cue-split ${CUE_APP_SOURCES} ${CUE_APP_HEADERS})
	target_link_libraries( dt-cue-split dt-cue-parser )
endif (ENABLE_SPLIT_TOOL)

# installation config
install(TARGETS dt-cue-parser LIBRARY DESTINATION ${LIB_INSTALL_DIR} )

if (ENABLE_SPLIT_TOOL)
	install(TARGETS dt-cue-split RUNTIME DESTINATION ${BIN_INSTALL_DIR})
endif (ENABLE_SPLIT_TOOL)

install(FILES ${CUE_LIBRARY_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}")
