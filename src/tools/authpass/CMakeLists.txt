set(AUTHPASS "authpass")

add_executable(${AUTHPASS} 
	authpass.c
	)

target_link_libraries(${AUTHPASS} web)

install(TARGETS ${AUTHPASS}
        EXPORT PROJECTTargets
        COMPONENT bin
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)