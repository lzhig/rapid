cmake_minimum_required (VERSION 2.8)
project(rapidlib)

SET(CMAKE_C_COMPILER g++)
SET(CMAKE_CXX_COMPILER g++)

if (NOT MSVC)
ADD_DEFINITIONS(
    -std=c++11 # Or -std=c++0x
    # Other flags
)
endif()

include_directories(include)

if ( MSVC )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/output/win32d )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/output/win32 )
	set( CMAKE_CXX_FLAGS_RELEASE "/MT")
	SET( CMAKE_CXX_FLAGS_DEBUG "/MTd /ZI /Od")
else( MSVC )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/output/linux32d )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/output/linux32 )
	set(CMAKE_CXX_FLAGS "-fPIC")
	SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb") 
	SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -g -Wall")
endif ( MSVC )

aux_source_directory(source DIR_SOURCE)
set(SOURCE_CPPS ${DIR_SOURCE})
add_library(rapidlib STATIC ${SOURCE_CPPS})

if ( NOT MSVC )
	set_target_properties(rapidlib PROPERTIES PREFIX "")
endif ()
