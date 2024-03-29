#
# Sets D3CAM_PROJECT_VERSION and D3CAM_PACKAGE_VERSION
#

if (CMAKE_BUILD_TYPE)
	string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
	if (BUILD_TYPE_UPPER MATCHES "DEBUG")
		add_definitions(-DCONFIG_DEBUG=1)
		option(USING_HEX_FORMAT "Show hex data in format" OFF)
		if (USING_HEX_FORMAT)
			add_definitions(-DCONFIG_DEBUG_HEX_FORMAT=1)
		endif()
	endif ()
else ()
	add_definitions(-DCONFIG_DEBUG=0)
endif ()

# Split a version number into separate components
#   version: the version number to split
#   major: variable name to store the major version in
#   minor: variable name to store the minor version in
#   patch: variable name to store the patch version in
#   extra: variable name to store a version suffix in
function(version_split version major minor patch extra)
    string(REGEX MATCH "([0-9]+)(\\.([0-9]+)(\\.([0-9]+)(.*)?)?)?" version_valid ${version})
    if(version_valid)
        string(REPLACE "." ";" version_split ${version})
        list(LENGTH version_split version_split_len)
        list(GET version_split 0 version_major)
        set(${major} ${version_major} PARENT_SCOPE)
        if (version_split_len GREATER "1")
            list(GET version_split 1 version_minor)
            set(${minor} ${version_minor} PARENT_SCOPE)
        endif()
        if (version_split_len GREATER "2")
            list(GET version_split 2 version_patch_extra)
            string(REGEX REPLACE "([0-9]+)(.*)" "\\1;\\2" version_patch_extra ${version_patch_extra})
            list(GET version_patch_extra 0 version_patch)
            set(${patch} ${version_patch} PARENT_SCOPE)
            list(GET version_patch_extra 1 version_extra)
            set(${extra} ${version_extra} PARENT_SCOPE)
        endif()
    else(version_valid)
        message(AUTHOR_WARNING "Bad version ${version}; falling back to 0 (have you made an initial release?)")
        set(${major} "0" PARENT_SCOPE)
        set(${minor} "0" PARENT_SCOPE)
        set(${patch} "0" PARENT_SCOPE)
        set(${extra} "" PARENT_SCOPE)
    endif(version_valid)
endfunction(version_split)

# Bump a version number. The final component of the version number is bumped.
# E.g.: 2.3 -> 2.4; 2 -> 3; 1.2.3 -> 1.2.4
# version_in the version to bump
# version_out variable name to store the bumped version in
function(version_bump version_in version_out)
    version_split(${version_in} major minor patch extra)
    if (patch MATCHES "^[0-9]+$")
        math(EXPR patch "${patch}+1")
        set(${version_out} "${major}.${minor}.${patch}" PARENT_SCOPE)
    elseif (minor MATCHES "^[0-9]+$")
        math(EXPR minor "${minor}+1")
        set(${version_out} "${major}.${minor}" PARENT_SCOPE)
    else()
        math(EXPR major "${major}+1")
        set(${version_out} "${major}" PARENT_SCOPE)
    endif()
endfunction(version_bump)

##########################
# get GIT_VERSION
# and GIT_FULL_VERSION
##########################
find_program(GIT_CMD git PATHS "C:/Program Files/Git/bin")
mark_as_advanced(GIT_CMD)
if (GIT_CMD)
    execute_process(COMMAND ${GIT_CMD} rev-parse --show-toplevel
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_TOPLEVEL
            ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if (GIT_CMD AND NOT "${GIT_TOPLEVEL}" STREQUAL "")
    execute_process(COMMAND ${GIT_CMD} rev-parse --short HEAD
            WORKING_DIRECTORY ${GIT_TOPLEVEL}
            OUTPUT_VARIABLE GIT_SHA1
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    #message(STATUS "GIT_SHA1: " ${GIT_SHA1})
    execute_process(COMMAND ${GIT_CMD} describe --match "*[0-9].[0-9]*" HEAD
            WORKING_DIRECTORY ${GIT_TOPLEVEL}
            OUTPUT_VARIABLE GIT_DESCRIBE
            ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "GIT_DESCRIBE: " ${GIT_DESCRIBE})

    if (GIT_DESCRIBE)
        string(REGEX REPLACE "v?([0-9.]+).*" "\\1" GIT_VERSION ${GIT_DESCRIBE})
        #message(STATUS "GIT_VERSION: " ${GIT_VERSION})

        # as package version we use the full version from git describe: 1.0.1+7+g8904c81
        if (GIT_DESCRIBE MATCHES ".*-g.*")
            # convert a git describe string to usable debian version, e.g. v1.0.1-7-g8904c81 to 1.0.1+7+g8904c81
            string(REGEX REPLACE "v?([0-9]*.[0-9.]*).*-([0-9]*)-([a-g0-9]*)" "\\1+\\2+\\3" GIT_FULL_VERSION ${GIT_DESCRIBE})
        else()
            # current HEAD is git tag (i.e. release), directly use the version
            set(GIT_FULL_VERSION ${GIT_VERSION})
        endif()
    else ()
        # no (suitable) tag found
        set(GIT_VERSION "0.0.0")
        # get number of commits in repo
        execute_process(COMMAND ${GIT_CMD} rev-list --count HEAD
                WORKING_DIRECTORY ${GIT_TOPLEVEL}
                OUTPUT_VARIABLE GIT_COMMIT_COUNT
                OUTPUT_STRIP_TRAILING_WHITESPACE)
        set(GIT_FULL_VERSION 0.0.0+${GIT_COMMIT_COUNT}+g${GIT_SHA1})
    endif ()
    # check if this is a shallow clone
    execute_process(COMMAND ${GIT_CMD} rev-parse --absolute-git-dir
                WORKING_DIRECTORY ${GIT_TOPLEVEL}
                OUTPUT_VARIABLE GIT_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (EXISTS "${GIT_DIR}/shallow")
        message(STATUS "Shallow git repo of depth ${GIT_COMMIT_COUNT}")
        set(GIT_REPO_IS_SHALLOW true)
    endif ()
endif ()


########################
# set D3CAM_PROJECT_VERSION
# priority:
#   - manually set
#   - git
#   - PROJECT_VERSION
########################
if (NOT D3CAM_PROJECT_VERSION)
    # set D3CAM_PROJECT_VERSION to MAJOR.MINOR.PATCH
    # D3CAM_PACKAGE_VERSION can have extra info
    if (GIT_VERSION)
        set(D3CAM_PROJECT_VERSION ${GIT_VERSION})
        set(D3CAM_PACKAGE_VERSION ${GIT_FULL_VERSION})
    elseif (PROJECT_VERSION)
        set(D3CAM_PROJECT_VERSION ${PROJECT_VERSION})
    else ()
        message(WARNING "D3CAM_PROJECT_VERSION not set. Defaulting to 0.0.0")
        set(D3CAM_PROJECT_VERSION "0.0.0")
    endif ()
endif ()
if (NOT D3CAM_PACKAGE_VERSION)
    message(WARNING "D3CAM_PACKAGE_VERSION not set! Falling back to D3CAM_PROJECT_VERSION (${D3CAM_PROJECT_VERSION})")
    set(D3CAM_PACKAGE_VERSION ${D3CAM_PROJECT_VERSION})
endif ()

# warn if versions don't match
if (GIT_VERSION AND NOT GIT_REPO_IS_SHALLOW AND NOT GIT_VERSION MATCHES ${D3CAM_PROJECT_VERSION})
    message(WARNING "Version from git (${GIT_VERSION}) doesn't match D3CAM_PROJECT_VERSION (${D3CAM_PROJECT_VERSION})")
endif()

message(STATUS "D3CAM_PROJECT_VERSION: " ${D3CAM_PROJECT_VERSION})
message(STATUS "D3CAM_PACKAGE_VERSION: " ${D3CAM_PACKAGE_VERSION})


version_split(${D3CAM_PROJECT_VERSION} PACKAGE_VERSION_MAJOR PACKAGE_VERSION_MINOR PACKAGE_VERSION_PATCH extra)
#message(STATUS "PACKAGE_VERSION_MAJOR: " ${PACKAGE_VERSION_MAJOR})
#message(STATUS "PACKAGE_VERSION_MINOR: " ${PACKAGE_VERSION_MINOR})
#message(STATUS "PACKAGE_VERSION_PATCH: " ${PACKAGE_VERSION_PATCH})

# generate an integer version number: major * 10000 + minor * 100 + patch
math(EXPR D3CAM_PROJECT_VERSION_INT "${PACKAGE_VERSION_MAJOR} * 10000 + ${PACKAGE_VERSION_MINOR} * 100 + ${PACKAGE_VERSION_PATCH}")

# make D3CAM_PROJECT_VERSION available as define in the project source
add_definitions(-DPROJECT_VERSION="${D3CAM_PROJECT_VERSION}")
add_definitions(-DPROJECT_VERSION_INT=${D3CAM_PROJECT_VERSION_INT})
add_definitions(-DPACKAGE_VERSION="${D3CAM_PACKAGE_VERSION}")
add_definitions(-DPACKAGE_VERSION_MAJOR=${PACKAGE_VERSION_MAJOR})
add_definitions(-DPACKAGE_VERSION_MINOR=${PACKAGE_VERSION_MINOR})
add_definitions(-DPACKAGE_VERSION_PATCH=${PACKAGE_VERSION_PATCH})

# set ABI version to major.minor, which will be used for the SOVERSION
set(abiversion "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")

# generate project_version.h if the .in files exist
# These files provide compile-time and runtime version information about your project.
if (EXISTS "${PROJECT_SOURCE_DIR}/cmake/project_version.h.in")
    string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)
    configure_file(cmake/project_version.h.in project_version.h @ONLY)
endif ()
