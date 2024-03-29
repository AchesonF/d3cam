set(EXE_TARGET "d3cam")

if (USING_WEB_SERVER)
	add_definitions(-DUSING_WEB_THREAD=1)

	add_subdirectory(libweb)
	add_subdirectory(tools)

	set(LIB_WEB_SERVER "web")
endif ()

if (USING_SPNG)
	add_subdirectory(libspng)

	set(LIB_SPNG "spng")
endif ()

if (USING_ZIP)
	add_definitions(-DZIP_SHARED)
	add_subdirectory(libzip)

	set(LIB_ZIP "zip")
endif ()

# add some include directories
#include_directories()


aux_source_directory(driver DIR_DRIVER)
aux_source_directory(device DIR_DEVICE)
aux_source_directory(interface DIR_INTERFACE)
aux_source_directory(function DIR_FUNCTION)
aux_source_directory(service DIR_SERVICE)
aux_source_directory(utility DIR_UTILITY)

add_executable(${EXE_TARGET}
	csv_main.c
	${DIR_DRIVER} 
	${DIR_DEVICE} 
	${DIR_INTERFACE} 
	${DIR_FUNCTION} 
	${DIR_SERVICE} 
	${DIR_UTILITY}
)

target_compile_features(${EXE_TARGET} PUBLIC cxx_std_11)

target_link_libraries(${EXE_TARGET}
	pthread
#	sqlite3
	dl
	MvCameraControl
	${libXML_LIBS}
	opencv_world
#	${OpenCV_LIBS}
	${LIB_WEB_SERVER}
	measureJetson
	${LIB_SPNG}
	${LIB_ZIP}
)


install(TARGETS ${EXE_TARGET}
        EXPORT PROJECTTargets
        COMPONENT bin
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/target
)


#set(TARGET_OUT_DIR ../target)
#add_custom_target(CopyTask)
#add_custom_command(TARGET CopyTask
#  POST_BUILD
#  COMMAND bash "-c" "sha256sum ${EXE_TARGET} >SHA256SUM"
#  COMMAND cp ${EXE_TARGET} ${TARGET_OUT_DIR}
#  COMMAND cp SHA256SUM ${TARGET_OUT_DIR}
#)

# add some other app directories
#add_subdirectory()
