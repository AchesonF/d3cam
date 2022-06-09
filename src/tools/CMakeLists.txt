
option(BUILD_APP_AUTHPASS "build auth password tool" ON)
if (BUILD_APP_AUTHPASS)
	add_subdirectory(authpass)
endif()

option(BUILD_APP_HTTPTOOL "build http tool" ON)
if (BUILD_APP_HTTPTOOL)
	add_subdirectory(httptool)
endif()

option(BUILD_APP_ESP "build esp compiler" ON)
if (BUILD_APP_ESP)
	add_subdirectory(esp)
endif ()

option(BUILD_CGI_TEST "build cgi program test" ON)
if (BUILD_CGI_TEST)
	add_subdirectory(cgi)
endif()

