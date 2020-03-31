cmake_minimum_required(VERSION 3.8)

project(QuadStackProject)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set (CMAKE_PREFIX_PATH "C:\\Qt\\Qt5.9.1\\5.9.1\\msvc2013_64")

find_package(Qt5 COMPONENTS Core Widgets OpenGL Gui REQUIRED)

# uncomment if g++ is desired under Windows 
#set(CMAKE_C_COMPILER "C:/MinGW/bin/gcc")
#set(CMAKE_CXX_COMPILER "C:/MinGW/bin/g++")

# environment variables
set(SOURCE core 
            model 
            ui 
            io 
            graphics)

file(GLOB_RECURSE SOURCE_CPP "src/*.cpp")

set(LIBS libs)

# set headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src ${LIBS})
#include_directories(${SOURCE} ${LIBS})

add_executable(QuadStackProject 
    quadstackapp.ui
    ${SOURCE_CPP}
    quadstackapp.cpp
    main.cpp 
    quadstackapp.qrc)

target_link_libraries (QuadStackProject Qt5::Widgets)
target_link_libraries (QuadStackProject Qt5::Core)
target_link_libraries (QuadStackProject Qt5::OpenGL)
target_link_libraries (QuadStackProject Qt5::Gui)