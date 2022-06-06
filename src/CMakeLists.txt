set(D3CAM "d3cam")

# add some include directories
#include_directories()


aux_source_directory(driver DIR_DRIVER)
aux_source_directory(device DIR_DEVICE)
aux_source_directory(interface DIR_INTERFACE)
aux_source_directory(function DIR_FUNCTION)
aux_source_directory(service DIR_SERVICE)
aux_source_directory(utility DIR_UTILITY)

add_executable(${D3CAM}
	csv_main.c
	${DIR_DRIVER} 
	${DIR_DEVICE} 
	${DIR_INTERFACE} 
	${DIR_FUNCTION} 
	${DIR_SERVICE} 
	${DIR_UTILITY}
)


target_link_libraries(${D3CAM}
	pthread
#	sqlite3
	MvCameraControl
)

install(TARGETS ${D3CAM}
        EXPORT PROJECTTargets
        COMPONENT bin
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


#set(TARGET_OUT_DIR ../target)
#add_custom_target(CopyTask)
#add_custom_command(TARGET CopyTask
#  POST_BUILD
#  COMMAND bash "-c" "sha256sum ${D3CAM} >SHA256SUM"
#  COMMAND cp ${D3CAM} ${TARGET_OUT_DIR}
#  COMMAND cp SHA256SUM ${TARGET_OUT_DIR}
#)

# add some other app directories
#add_subdirectory()
