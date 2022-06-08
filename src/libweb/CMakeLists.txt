set(LIB_WEB web)

file(GLOB SRC_CFG cfg/*.c)
file(GLOB SRC_ESP esp/*.c)
file(GLOB SRC_HTTP http/*.c)
file(GLOB SRC_HANDLER handler/*.c)
file(GLOB SRC_MPR mpr/*.c)
file(GLOB SRC_PCRE pcre/*.c)
file(GLOB SRC_SQLITE sqlite/*.c)
file(GLOB SRC_SSL ssl/*.c)

#include_directories(
#	esp
#	http
#	mpr
#	pcre
#	sqlite
#)

add_library(${LIB_WEB} SHARED 
	${SRC_CFG} 
	${SRC_ESP} 
	${SRC_HTTP} 
	${SRC_HANDLER} 
	${SRC_MPR} 
	${SRC_PCRE} 
	${SRC_SQLITE} 
	${SRC_SSL}
)

target_link_libraries(${LIB_WEB} pthread dl ssl crypto )

install(TARGETS ${LIB_WEB}
        EXPORT PROJECTTargets
        COMPONENT bin
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)