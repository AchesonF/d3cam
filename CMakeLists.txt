
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "FATAL: In-source builds are not allowed.
		You should create a separate directory for build files.")
endif()

cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

project(D3WEB VERSION 1.0.0 LANGUAGES CXX C)

message(STATUS "PROJECT_NAME: " ${PROJECT_NAME})
message(STATUS "PROJECT_VERSION: " ${PROJECT_VERSION})

include(cmake/project_version.cmake)
include(GNUInstallDirs)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type: Debug or Release" FORCE)
endif ()

message(STATUS "CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")

#set(PREFIX_WORK_MACHINE "arm-linux-" CACHE STRING "Compiler's prefix for working machine.")

set(CMAKE_C_COMPILER ${PREFIX_WORK_MACHINE}gcc)
set(CMAKE_CXX_COMPILER ${PREFIX_WORK_MACHINE}g++)
set(CMAKE_OBJCOPY ${PREFIX_WORK_MACHINE}objcopy)
set(CMAKE_LINKER ${GNU_MAPREFIX_WORK_MACHINECHINE}ld)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

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

if ("${ARCHITECTURE}" STREQUAL "aarch64")
	set(DIR_LIBS_MVS "/opt/MVS/lib/aarch64/")
elseif ("${ARCHITECTURE}" STREQUAL "x86_64")
	set(DIR_LIBS_MVS "/opt/MVS/lib/64/")
else ()
	message(FATAL_ERROR "Not support machine: " ${ARCHITECTURE})
endif ()

find_package(OpenCV REQUIRED)
if (OpenCV_FOUND)
	message(STATUS "OpenCV library status:")
	message(STATUS "    version: ${OpenCV_VERSION}")
	message(STATUS "    libraries: ${OpenCV_LIBS}")
	message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
else (OpenCV_FOUND)
	message(FATAL_ERROR "OpenCV not found. Please install first.")
endif ()

set(libXML_INCLUDE_DIRS "/usr/include/libxml2")
set(libXML_LIBS "xml2")

link_directories(
	${DIR_LIBS_MVS}
	"libs"
)

option(USING_WEB_SERVER "Add web thread" OFF)
if (USING_WEB_SERVER)
	set(INC_WEB_SERVER 
		"include/web"
		"src/libweb/esp"
		"src/libweb/http"
		"src/libweb/mpr"
		"src/libweb/pcre"
		"src/libweb/sqlite"
	)
endif ()

option(USING_SPNG "Add png maker" ON)
if (USING_SPNG)
	set(INC_SPNG
		"src/libspng"
	)
endif ()

# add include directories here
include_directories(
	"include"
	"include/mvs"
	"include/gev"
	"include/so"
	"include/pointcloud"
	${INC_WEB_SERVER}
	${CMAKE_BINARY_DIR}
	${libXML_INCLUDE_DIRS}
	${OpenCV_INCLUDE_DIRS}
	${INC_SPNG}
)

add_subdirectory(src)

