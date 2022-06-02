cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

project(D3Cam VERSION 1.0.0 LANGUAGES CXX C)

message(STATUS "PROJECT_NAME: " ${PROJECT_NAME})
message(STATUS "PROJECT_VERSION: " ${PROJECT_VERSION})

include(cmake/project_version.cmake)
include(GNUInstallDirs)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RELEASE CACHE STRING "Build type: DEBUG or RELEASE" FORCE)
endif ()

message(STATUS "CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")

#set(PREFIX_WORK_MACHINE "arm-linux-" CACHE STRING "Compiler's prefix for working machine.")

set(CMAKE_C_COMPILER ${PREFIX_WORK_MACHINE}gcc)
set(CMAKE_CXX_COMPILER ${PREFIX_WORK_MACHINE}g++)
set(CMAKE_OBJCOPY ${PREFIX_WORK_MACHINE}objcopy)
set(CMAKE_LINKER ${GNU_MAPREFIX_WORK_MACHINECHINE}ld)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "./usr/local" CACHE PATH "Set default install prefix." FORCE)
endif ()

# try to get architecture from compiler
EXECUTE_PROCESS(COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine 
	COMMAND tr -d '\n' 
	OUTPUT_VARIABLE CXX_MACHINE
)

string(REGEX REPLACE "([a-zA-Z_0-9]+).*" "\\1" ARCHITECTURE ${CXX_MACHINE})
message(STATUS "Compile for Machine: " ${ARCHITECTURE})


# add include directories here
include_directories(
	include
	${CMAKE_BINARY_DIR}
)

add_subdirectory(src)

