project(ImageWidget)
add_compile_options("/std:c++17")
cmake_minimum_required( VERSION 2.8 )

SET(OPENCV_DIR ./ CACHE PATH "OPENCV_DIR")

INCLUDE(${OPENCV_DIR}/OpenCVConfig.cmake)

include_directories( ${OpenCV_INCLUDE_DIRS} )


#add qt lib
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
#include_directories(${Qt5_include_dirs})
include_directories(${Qt5Widgets_INCLUDE_DIRS})
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set( CMAKE_INCLUDE_CURRENT_DIR ON )
#DEBUG生成的结尾为D
SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "" FORCE)

set(HEADERS 
    include/ImageWidget.hxx
    include/ROIDialog.hxx
)
set(SOURCES 
    src/ImageWidget.cxx
    src/main.cxx
    src/ROIDialog.cxx
)
set(RESOURCES 
    rcc/ImageWidget.qrc
)
include_directories(include)
add_library(${PROJECT_NAME}  ${HEADERS} ${SOURCES} ${RESOURCES})

install (TARGETS ImageWidget
LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/lib
ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/lib
)

install(FILES ${HEADERS} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/include/)
