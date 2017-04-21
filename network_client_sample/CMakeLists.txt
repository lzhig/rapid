cmake_minimum_required (VERSION 2.8)
project(network_client_sample)

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
	${PROJECT_SOURCE_DIR}/../networkengine/include
	)

if ( MSVC )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/output/win32d )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/output/win32 )
else()
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/output/linux32d )
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/output/linux32 )
endif ( MSVC )

aux_source_directory(source DIR_SOURCE)
set(SOURCE_CPPS ${DIR_SOURCE})

add_library(network_client_sample SHARED ${SOURCE_CPPS})

if ( MSVC )
	SET(LINK_LIBRARY
		optimized ${PROJECT_SOURCE_DIR}/../rapidlib/output/win32/rapidlib.lib 
		debug ${PROJECT_SOURCE_DIR}/../rapidlib/output/win32d/rapidlib.lib
		)
else()
	SET(LINK_LIBRARY
		optimized ${PROJECT_SOURCE_DIR}/../rapidlib/output/linux32/rapidlib.a 
		debug ${PROJECT_SOURCE_DIR}/../rapidlib/output/linux32d/rapidlib.a
		)
	set_target_properties(network_client_sample PROPERTIES PREFIX "")
endif (MSVC)

target_link_libraries(network_client_sample ${LINK_LIBRARY})
