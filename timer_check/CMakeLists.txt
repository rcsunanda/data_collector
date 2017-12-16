cmake_minimum_required (VERSION 2.8)

project ("Timer Check App")

#set a include & linker paths
set (COMMON_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../lib/" )

include_directories(${COMMON_LIBRARY_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

aux_source_directory(${COMMON_LIBRARY_PATH} COMMON_SOURCE_FILES)
#aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} COMMON_SOURCE_FILES)	#remove because this directory contains another main file

#compiler flags
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -Wall -ggdb")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -std=c++11")

set (TARGET1 timer_check_app)
add_executable (${TARGET1} timer_check_app_main.cpp ${COMMON_SOURCE_FILES})
set_target_properties (${TARGET1} PROPERTIES COMPILE_FLAGS "-DBOOST_LOG_DYN_LINK")
target_link_libraries(${TARGET1} rt pthread boost_system boost_thread boost_log boost_log_setup)


#print some useful in-built variables
message (STATUS "========================================")
message (STATUS "Compiler used: ${CMAKE_CXX_COMPILER}")
message (STATUS "CMake build type: ${CMAKE_BUILD_TYPE}")
message (STATUS "CMake default build flags: ${CMAKE_CXX_FLAGS}")
message (STATUS "CMake debug build flags: ${CMAKE_CXX_FLAGS_DEBUG}")
message (STATUS "CMake release build flags: ${CMAKE_CXX_FLAGS_RELEASE}")
message (STATUS "CMake linker flags: ${CMAKE_EXE_LINKER_FLAGS}")
message (STATUS "========================================")
