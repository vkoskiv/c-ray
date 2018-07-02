cmake_minimum_required (VERSION 3.5)
project (c-ray)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMKE_C_FLAGS_RELEASE "-O3")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

FILE(GLOB CSources src/*.c)
add_executable(c-ray ${CSources})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${c-ray_SOURCE_DIR}/cmake")

find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} -lpthread -lm ${SDL2_LIBRARY})
