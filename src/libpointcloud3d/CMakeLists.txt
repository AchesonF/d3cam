
#project(csv3DMeasureJetsonNx LANGUAGES CXX CUDA)

set(LIB_TARGET "pointcloud3d")

add_library(${LIB_TARGET} SHARED
  CalibFileInfo.cpp
  CalibFileInfo.hpp
  CameraParam.cpp
  CameraParam.h
  csvCreatePoint3D.cpp
  csvCreatePoint3D.hpp
  CsvErrorType.hpp
  CsvMeasure3D.cpp
  CsvMeasure3D.hpp
  csvPoint3DCloud.hpp
  CsvStereoMatching.cpp
  CsvStereoMatching.hpp
  CsvType.hpp
  CsvVersion.h
  ExtrinsicParam.cpp
  ExtrinsicParam.h
  IntrinsicParam.cpp
  IntrinsicParam.h
  MacroCommon.h
  tinystr.cpp
  tinystr.h
  tinyxml.cpp
  tinyxml.h
  tinyxmlerror.cpp
  tinyxmlparser.cpp
  VisionHeadParam.cpp
  VisionHeadParam.h
)

# build with -std=c++11
target_compile_features(${LIB_TARGET} PUBLIC cxx_std_11)

# Add OpenCV headers location to your include paths
include_directories($(PROJECT_SOURCE_DIR) ${OpenCV_INCLUDE_DIRS})


# Link your application with OpenCV libraries
target_link_libraries(${LIB_TARGET} ${OpenCV_LIBS} pthread)

set_target_properties( ${LIB_TARGET}
                       PROPERTIES CUDA_SEPARABLE_COMPILATION ON
                      )

