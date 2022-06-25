set(HTTPTOOL "httptool")

add_executable(${HTTPTOOL} 
	httptool.c
	)

target_link_libraries(${HTTPTOOL} web)

install(TARGETS ${HTTPTOOL}
        EXPORT PROJECTTargets
        COMPONENT bin
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)