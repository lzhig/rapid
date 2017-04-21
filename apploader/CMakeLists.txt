cmake_minimum_required (VERSION 2.8)
project(apploader)

if (NOT MSVC)
SET(CMAKE_C_COMPILER g++)
SET(CMAKE_CXX_COMPILER g++)
ADD_DEFINITIONS(
    -std=c++11 # Or -std=c++0x
    # Other flags
)
endif()

include_directories(include ${PROJECT_SOURCE_DIR}/../rapidlib/include)

if ( MSVC )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/output/win32d )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/output/win32 )
	set( CMAKE_CXX_FLAGS_RELEASE "/MT")
	SET( CMAKE_CXX_FLAGS_DEBUG "/MTd /ZI /Od")
else()
	SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb") 
	SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -g -Wall")
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/output/linux32d )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/output/linux32 )
endif ( MSVC )

aux_source_directory(source DIR_SOURCE)
set(SOURCE_CPPS ${DIR_SOURCE})

add_executable(apploader ${SOURCE_CPPS})

if ( MSVC )
	SET(LINK_LIBRARY
		optimized ${PROJECT_SOURCE_DIR}/../rapidlib/output/win32/rapidlib.lib 
		debug ${PROJECT_SOURCE_DIR}/../rapidlib/output/win32d/rapidlib.lib
		)
	target_link_libraries(apploader ${LINK_LIBRARY})
else()
	SET(LINK_LIBRARY
		optimized ${PROJECT_SOURCE_DIR}/../rapidlib/output/linux32/rapidlib.a 
		debug ${PROJECT_SOURCE_DIR}/../rapidlib/output/linux32d/rapidlib.a)
	target_link_libraries(apploader ${LINK_LIBRARY} ${CMAKE_DL_LIBS} pthread)
endif (MSVC)
