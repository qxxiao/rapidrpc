cmake_minimum_required(VERSION 3.20)

project(rapidrpc VERSION 1.0)
set(project_dir ${CMAKE_CURRENT_SOURCE_DIR})


set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_FLAGS "-g -O0 -Wall -Wno-deprecated -Wno-unused-but-set-variable")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



# set cmake  binary directory
set(EXECUTABLE_OUTPUT_PATH ${project_dir}/bin)
include_directories(${project_dir}/include)

# add sub directories
add_subdirectory(test)