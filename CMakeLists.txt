cmake_minimum_required(VERSION 3.5)

project(torinoscan VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MAIN_EXEC_NAME ${CMAKE_PROJECT_NAME})

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(yaml-cpp)
find_package(PkgConfig)
pkg_check_modules(LIBRARY_ICEBLOCK libiceblock IMPORTED_TARGET)

if (NOT LIBRARY_ICEBLOCK_FOUND)
  message(FATAL_ERROR "libiceblock required to build.")
endif()

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        paramwidgetgroup.cpp
        paramwidgetgroup.h
        actuatorwidgetgroup.cpp
        actuatorwidgetgroup.h
        protocoliface.cpp
        protocoliface.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${MAIN_EXEC_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(${MAIN_EXEC_NAME}
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(${MAIN_EXEC_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    yaml-cpp
    iceblock
    )

set_target_properties(${MAIN_EXEC_NAME} PROPERTIES
    #MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${MAIN_EXEC_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${MAIN_EXEC_NAME})
endif()

