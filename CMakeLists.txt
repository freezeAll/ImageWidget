project(ImageWidget)
add_compile_options("/std:c++17")
cmake_minimum_required( VERSION 2.8 )

SET(OPENCV_DIR ./ CACHE PATH "OPENCV_DIR")

INCLUDE(${OPENCV_DIR}/OpenCVConfig.cmake)

include_directories( ${OpenCV_INCLUDE_DIRS} )


#add qt lib

IF(USE_QML)
   find_package(Qt5 COMPONENTS Core Gui Qml Quick REQUIRED)
ELSE()
   find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
ENDIF()
#include_directories(${Qt5_include_dirs})
include_directories(${Qt5Widgets_INCLUDE_DIRS})
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set( CMAKE_INCLUDE_CURRENT_DIR ON )
#DEBUG���ɵĽ�βΪD
SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "" FORCE)
SET(USE_QML 0 CACHE BOOL 0)
SET(BUILD_SHARED_LIBS 1 CACHE BOOL 1)
add_definitions("-DIMAGEWIDGET_LIB")
IF(USE_QML)
    add_definitions("-DIMAGEWIDGET_QML")
ENDIF()
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

IF(USE_QML)
    SET(OUT_NAME ${PROJECT_NAME}QML)
ELSE()
	SET(OUT_NAME ${PROJECT_NAME})
ENDIF()

add_library(${OUT_NAME}  ${HEADERS} ${SOURCES} ${RESOURCES})

IF(USE_QML)
    target_link_libraries(${OUT_NAME}  Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick ${OpenCV_LIBS})
ELSE()
   target_link_libraries(${OUT_NAME}  Qt5::Core Qt5::Gui Qt5::Widgets ${OpenCV_LIBS})
ENDIF()
install (TARGETS ${OUT_NAME}
LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/lib
ARCHIVE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/lib
RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/bin
)

install(FILES ${HEADERS} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/include/)
IF(USE_QML)
    install(FILES src/qmldir DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/bin/)
ENDIF()