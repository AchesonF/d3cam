# general cpack variables
set(CPACK_PACKAGE_CONTACT "csv <csv@csv.com>")
set(CPACK_PACKAGE_VENDOR "csv, Shenzhen, China")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "csv highspeed 3d package from cmake.")
SET(CPACK_PACKAGE_VERSION_MAJOR ${PACKAGE_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${PACKAGE_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${PACKAGE_VERSION_PATCH})

# CPACK_PACKAGE_VERSION
if (PACKAGE_VERSION)
    set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
else ()
    if (PRJ_VERSION)
        set(CPACK_PACKAGE_VERSION ${PRJ_VERSION})
    elseif (PROJECT_VERSION)
        set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    endif ()
endif ()

# add date stamp("%Y%m%d+%H%M%S") to CPACK_PACKAGE_VERSION
string(TIMESTAMP STAMP "%Y%m%d")
set(CPACK_PACKAGE_VERSION "${D3CAM_PACKAGE_VERSION}-${STAMP}")

###############################
# debian package specific stuff
###############################
set(CPACK_GENERATOR "DEB")
#set(CPACK_DEBIAN_PACKAGE_DEBUG ON)

# default to generating one debian package per COMPONENT on cmake >= 3.6
if (NOT DEFINED CPACK_DEB_COMPONENT_INSTALL)
  if (NOT (CMAKE_VERSION VERSION_LESS "3.6"))
    set(CPACK_DEB_COMPONENT_INSTALL ON)
  endif ()
endif ()
if (CPACK_DEB_COMPONENT_INSTALL)
  message(STATUS "CPACK_DEB_COMPONENT_INSTALL is on")
  set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
endif ()

if (NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    find_program(DPKG_CMD dpkg)
    mark_as_advanced(DPKG_CMD)
    if (NOT DPKG_CMD)
        message(STATUS "Can not find dpkg in your path, default to i386.")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
    else ()
        execute_process(COMMAND "${DPKG_CMD}" --print-architecture
                OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
                OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif ()
endif ()
message(STATUS "CPACK_PACKAGE_VERSION: " ${CPACK_PACKAGE_VERSION})


# package name defaults to lower case of project name with _ replaced by -
if (NOT CPACK_PACKAGE_NAME)
    string(TOLOWER "${PROJECT_NAME}" PROJECT_NAME_LOWER)
    string(REPLACE "_" "-" CPACK_PACKAGE_NAME "${PROJECT_NAME_LOWER}")
endif ()
message(STATUS "CPACK_PACKAGE_NAME: " ${CPACK_PACKAGE_NAME})



set(CPACK_DEBIAN_BIN_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")

if (CMAKE_VERSION VERSION_LESS "3.6.0")
  set(CPACK_DEBIAN_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
  message(STATUS "setting CPACK_PACKAGE_FILE_NAME: " ${CPACK_PACKAGE_FILE_NAME})
else ()
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
endif ()



# ask if example sysctl config should be applied when installing this as debian package
set(CONFIG_FILE "${PROJECT_BINARY_DIR}/cpack/config")
set(POSTINST_FILE "${PROJECT_BINARY_DIR}/cpack/postinst")
set(POSTRM_FILE "${PROJECT_BINARY_DIR}/cpack/postrm")
set(PREINST_FILE "${PROJECT_BINARY_DIR}/cpack/preinst")
set(PRERM_FILE "${PROJECT_BINARY_DIR}/cpack/prerm")
set(TEMPLATES_FILE "${PROJECT_BINARY_DIR}/cpack/templates")
configure_file(${PROJECT_SOURCE_DIR}/target/cpack/config.in ${CONFIG_FILE})
configure_file(${PROJECT_SOURCE_DIR}/target/cpack/postinst.in ${POSTINST_FILE})
configure_file(${PROJECT_SOURCE_DIR}/target/cpack/postrm.in ${POSTRM_FILE})
configure_file(${PROJECT_SOURCE_DIR}/target/cpack/preinst.in ${PREINST_FILE})
configure_file(${PROJECT_SOURCE_DIR}/target/cpack/prerm.in ${PRERM_FILE})
configure_file(${PROJECT_SOURCE_DIR}/target/cpack/templates.in ${TEMPLATES_FILE})
execute_process(COMMAND chmod 755 "${CONFIG_FILE}")
execute_process(COMMAND chmod 755 "${POSTINST_FILE}")
execute_process(COMMAND chmod 755 "${POSTRM_FILE}")
execute_process(COMMAND chmod 755 "${PREINST_FILE}")
execute_process(COMMAND chmod 755 "${PRERM_FILE}")
execute_process(COMMAND chmod 644 "${TEMPLATES_FILE}")

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA};${PREINST_FILE};${POSTINST_FILE};${PRERM_FILE};${POSTRM_FILE};${TEMPLATES_FILE};${CONFIG_FILE}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "ftp-upload")

# make dev and gui components depend on bin with rpfilter.conf
set(CPACK_COMPONENT_DEV_DEPENDS "bin")
set(CPACK_COMPONENT_GUI_DEPENDS "bin")

include(CPack)

