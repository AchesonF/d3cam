set(ESP "esp")

add_executable(${ESP} 
	esp.c
	)

target_link_libraries( ${ESP} web)

install(TARGETS ${ESP}
        EXPORT PROJECTTargets
        COMPONENT bin
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
