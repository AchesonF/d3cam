set(CGI "cgiprom.cgi")

add_executable(${CGI} 
	cgiProgram.c
	)

target_link_libraries( ${CGI} )

install(TARGETS ${CGI}
        EXPORT PROJECTTargets
        COMPONENT bin
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/cgi-bin
)
