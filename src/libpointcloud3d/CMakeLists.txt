set(LIB_POINTCLOUD3D pointcloud3d)

add_library(${LIB_POINTCLOUD3D} SHARED
	csvCreatePoint3D.cpp
	csvCreatePoint3D.hpp
	csvPoint3DCloud.hpp
)

target_compile_features(${LIB_POINTCLOUD3D} PUBLIC cxx_std_11)

#arget_link_libraries(${LIB_POINTCLOUD3D} pthread )

install(TARGETS ${LIB_POINTCLOUD3D}
        EXPORT PROJECTTargets
        COMPONENT bin
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
