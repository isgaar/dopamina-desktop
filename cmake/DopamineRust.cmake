# Location: cmake/DopamineRust.cmake
# Function: Imports the Rust core crate into the CMake build through
# CXX-Qt and Corrosion, exporting generated QObject headers to C++.
# All Rights Reserved isgaar

include(FetchContent)

find_package(CxxQt QUIET)
if(NOT CxxQt_FOUND)
    FetchContent_Declare(
        CxxQt
        GIT_REPOSITORY https://github.com/KDAB/cxx-qt-cmake.git
        GIT_TAG 0.10.0
    )
    FetchContent_MakeAvailable(CxxQt)
endif()

cxx_qt_import_crate(
    MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/rust/Cargo.toml
    CRATES dopamine_desktop
    QT_MODULES Qt::Core Qt::Gui Qt::Qml
)
