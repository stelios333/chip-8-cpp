cmake_minimum_required(VERSION 3.10)
project(Chip8_Emulator)

set(CMAKE_CXX_STANDARD 23)

if($ENV{TRAVIS})
    if($ENV{TRAVIS} STREQUAL "true")
        message(STATUS "Building on Travis-CI.")
        set(IS_TRAVIS_BUILD TRUE)
    endif()
endif()


find_package(SDL2_ttf REQUIRED)
find_package(SDL2 REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic-errors \
-Wall -Wextra -Wconversion -Wsign-conversion")

set(SOURCE_FILES src/main.cpp src/chip8.cpp src/audio.cpp src/chip8.h src/audio.h)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SDL2_LIBRARIES} SDL2_ttf::SDL2_ttf)
