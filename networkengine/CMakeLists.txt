cmake_minimum_required (VERSION 2.8)
project(networkengine)

SET(CMAKE_C_COMPILER g++)
SET(CMAKE_CXX_COMPILER g++)

if (MSVC)
ADD_DEFINITIONS(
	-DSERVICE_EXPORTS
)
else()
ADD_DEFINITIONS(
    -std=c++11 # Or -std=c++0x
	-DSERVICE_EXPORTS
)
endif()

include_directories(include
	${PROJECT_SOURCE_DIR}/../rapidlib/include
	${PROJECT_SOURCE_DIR}/dep/libevent/inc
	)

if ( MSVC )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/output/win32d )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/output/win32 )
	include_directories(${PROJECT_SOURCE_DIR}/dep/libevent/inc/win32)
	set( CMAKE_CXX_FLAGS_RELEASE "/MT")
	SET( CMAKE_CXX_FLAGS_DEBUG "/MTd /ZI /Od")
else()
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/output/linux32d )
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/output/linux32 )
	include_directories(${PROJECT_SOURCE_DIR}/dep/libevent/inc/linux)
	set(CMAKE_CXX_FLAGS "-fPIC")
	SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb") 
	SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -g -Wall")
endif ( MSVC )

aux_source_directory(source DIR_SOURCE)
set(SOURCE_CPPS ${DIR_SOURCE})

add_library(networkengine SHARED ${SOURCE_CPPS})

if ( MSVC )
	SET(LINK_LIBRARY
		optimized ${PROJECT_SOURCE_DIR}/../rapidlib/output/win32/rapidlib.lib 
		debug ${PROJECT_SOURCE_DIR}/../rapidlib/output/win32d/rapidlib.lib
		ws2_32.lib
		)
	set(LIBEVENT_LIB
		optimized ${PROJECT_SOURCE_DIR}/../../rapid_dep/libevent/lib/win32/event.lib
		debug ${PROJECT_SOURCE_DIR}/../../rapid_dep/libevent/lib/win32d/event.lib)
else()
	SET(LINK_LIBRARY
		optimized ${PROJECT_SOURCE_DIR}/../rapidlib/output/linux32/rapidlib.a 
		debug ${PROJECT_SOURCE_DIR}/../rapidlib/output/linux32d/rapidlib.a)
	set(LIBEVENT_LIB
		${PROJECT_SOURCE_DIR}/dep/libevent/lib/linux32/libevent.a
		${PROJECT_SOURCE_DIR}/dep/libevent/lib/linux32/libevent_pthreads.a
		pthread
		rt)
	set_target_properties(networkengine PROPERTIES PREFIX "")
endif (MSVC)

target_link_libraries(networkengine ${LINK_LIBRARY} ${LIBEVENT_LIB})
