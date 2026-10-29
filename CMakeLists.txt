# Location: CMakeLists.txt
# Function: Master CMake project definition for Dopamine native desktop
# client orchestrating Qt 6 Quick, C++ modules, and target linking.
# All Rights Reserved isgaar

cmake_minimum_required(VERSION 3.20)
project(Dopamine VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(PREPEND CMAKE_PREFIX_PATH "/home/ismael/.local/usr")
find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Qml Quick Network Widgets Sql Test Concurrent
)
if(UNIX AND NOT APPLE)
    find_package(Qt6 COMPONENTS DBus QUIET)
endif()
find_package(Qt6Multimedia REQUIRED
    PATHS "/home/ismael/.local/usr/lib64/cmake/Qt6Multimedia"
    NO_DEFAULT_PATH
)

include(cmake/DopamineRust.cmake)
qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
include(cmake/DopamineSources.cmake)
include(cmake/DopamineErrors.cmake)
include(cmake/DopamineQml.cmake)
qt_add_executable(dopamine WIN32 ${PROJECT_SOURCES})

qt_add_qml_module(dopamine
    URI Dopamine.Core
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES ${DOPAMINE_QML_FILES}
    RESOURCES ${DOPAMINE_RESOURCES}
)

target_link_libraries(dopamine PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Network Qt6::Widgets Qt6::Sql
    Qt6::Multimedia Qt6::Concurrent dopamine_desktop
)
if(TARGET Qt6::DBus)
    target_link_libraries(dopamine PRIVATE Qt6::DBus)
    target_compile_definitions(dopamine PRIVATE DOPAMINE_HAS_DBUS)
    target_sources(dopamine PRIVATE
        src/core/platform/media/mpris/MprisRootAdaptor.cpp
        src/core/platform/media/mpris/MprisRootAdaptor.h
        src/core/platform/media/mpris/MprisPlayerAdaptor.cpp
        src/core/platform/media/mpris/MprisPlayerAdaptor.h
        src/core/platform/media/mpris/MprisPlayerService.cpp
        src/core/platform/media/mpris/MprisPlayerService.h
    )
endif()

target_include_directories(dopamine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

install(TARGETS dopamine DESTINATION bin)

include(cmake/DopamineTests.cmake)

