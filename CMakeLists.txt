cmake_minimum_required(VERSION 3.20)
project(FusionBrowser VERSION 9.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS
    Core Gui Widgets WebEngineWidgets WebEngineCore Sql Network Concurrent)

set(SOURCES
    src/main.cpp
    src/database.h          src/database.cpp
    src/settings.h          src/settings.cpp
    src/themes.h            src/themes.cpp
    src/urlbar.h            src/urlbar.cpp
    src/loadbar.h           src/loadbar.cpp
    src/browsertab.h        src/browsertab.cpp
    src/tabbar.h            src/tabbar.cpp
    src/homepage.h          src/homepage.cpp
    src/sidepanel.h         src/sidepanel.cpp
    src/downloadspanel.h    src/downloadspanel.cpp
    src/settingspanel.h     src/settingspanel.cpp
    src/extensions.h        src/extensions.cpp
    src/importer.h          src/importer.cpp
    src/onboarding.h        src/onboarding.cpp
    src/mainwindow.h        src/mainwindow.cpp
    resources/fusion.qrc
)

if(WIN32)
    add_executable(Fusion WIN32 ${SOURCES} resources/fusion.rc)
else()
    add_executable(Fusion ${SOURCES})
endif()

target_link_libraries(Fusion PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets
    Qt6::WebEngineWidgets Qt6::WebEngineCore
    Qt6::Sql Qt6::Network Qt6::Concurrent)

target_include_directories(Fusion PRIVATE src)

if(MSVC)
    target_compile_options(Fusion PRIVATE /W3 /utf-8)
endif()

set_target_properties(Fusion PROPERTIES
    OUTPUT_NAME "Fusion"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
