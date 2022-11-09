
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "FATAL: In-source builds are not allowed.
		You should create a separate directory for build files.")
endif()

cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

#project(D3CAM VERSION 1.0.0 LANGUAGES CXX C CUDA)
project(D3CAM VERSION 1.0.0 LANGUAGES CXX C)

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
	set(DIR_LIBS_3D "libs/")
elseif ("${ARCHITECTURE}" STREQUAL "x86_64")
	set(DIR_LIBS_MVS "/opt/MVS/lib/64/")
	set(DIR_LIBS_3D "libs/x64/")
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
set(LIB_XML2 "xml2")

link_directories(
	${DIR_LIBS_MVS}
	${DIR_LIBS_3D}
)

option(USING_CAMS_GX "using Daheng galaxy cams" ON)
option(USING_CAMS_HK "using Hikang cams" OFF)
if (USING_CAMS_GX)
	add_definitions(-DUSE_GX_CAMS)
	set(LIB_GX_CAMS "gxiapi")
	set(USING_CAMS_HK OFF CACHE BOOL "No hk cams" FORCE)
else ()
	if (USING_CAMS_HK)
		add_definitions(-DUSE_HK_CAMS)
		set(LIB_HK_CAMS "MvCameraControl")
		set(USING_CAMS_GX OFF CACHE BOOL "No gx cams" FORCE)
	else ()
		add_definitions(-DUSE_GX_CAMS)
		set(LIB_GX_CAMS "gxiapi")
		set(USING_CAMS_GX ON CACHE BOOL "Use gx cams" FORCE)
	endif ()
endif ()


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

# add include directories here
include_directories(
	"include"
	"include/mvs"
	"include/galaxy"
	"include/gev"
	"include/so"
	"include/3d"
	${INC_WEB_SERVER}
	${CMAKE_BINARY_DIR}
	${libXML_INCLUDE_DIRS}
	${OpenCV_INCLUDE_DIRS}
)

add_subdirectory(src)

install(FILES ${CMAKE_SOURCE_DIR}/target/CSV_Cali_DaHengCamera.xml COMPONENT bin DESTINATION /opt/d3cam/config/)
install(FILES ${CMAKE_SOURCE_DIR}/target/web_conf/auth.conf COMPONENT bin DESTINATION /opt/d3cam/config/)
install(FILES ${CMAKE_SOURCE_DIR}/target/web_conf/webapp.conf COMPONENT bin DESTINATION /opt/d3cam/config/)
install(FILES ${CMAKE_SOURCE_DIR}/target/web_conf/webapp-all.conf COMPONENT bin DESTINATION /opt/d3cam/config/)
install(FILES ${CMAKE_SOURCE_DIR}/target/web_conf/csv.crt COMPONENT bin DESTINATION /opt/d3cam/sets/)
install(FILES ${CMAKE_SOURCE_DIR}/target/web_conf/csv.key COMPONENT bin DESTINATION /opt/d3cam/sets/)
install(FILES ${CMAKE_SOURCE_DIR}/target/CS-3D001-3MHS.zip COMPONENT bin DESTINATION /opt/d3cam/sets/)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/target/public/ COMPONENT bin DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/d3cam/)

include(cmake/package_deb.cmake)
