set(LIB_ZIP zip)

add_library(${LIB_ZIP} SHARED
	zip.c
	zip.h
	miniz.h
)

install(TARGETS ${LIB_ZIP}
        EXPORT PROJECTTargets
        COMPONENT bin
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)