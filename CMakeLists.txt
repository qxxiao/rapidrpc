cmake_minimum_required(VERSION 3.20)

project(rapidrpc VERSION 1.0)
set(project_dir ${CMAKE_CURRENT_SOURCE_DIR})


set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_FLAGS "-g -O0 -Wall -Wno-format-security -Wno-unused-but-set-variable")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_PARALLEL_LEVEL 10)



# set cmake  directory
set(EXECUTABLE_OUTPUT_PATH ${project_dir}/bin) # or set properties in add_executable RUNTIME_OUTPUT_DIRECTORY
set (LIBRARY_OUTPUT_PATH ${project_dir}/lib)   # or set properties in add_library LIBRARY_OUTPUT_DIRECTORY
include_directories(${project_dir}/include)
include_directories(${project_dir}/pb)

# add library
file(GLOB_RECURSE CC_FILES ${project_dir}/src/*.cc)
add_library(${PROJECT_NAME} STATIC ${CC_FILES})
# install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION /path/to/your/lib)

# add sub directories
add_subdirectory(test)

# 指定头文件目录的安装路径
set(CMAKE_INSTALL_PREFIX /usr/local)
install(DIRECTORY ${project_dir}/include/
        DESTINATION include
        COMPONENT headers)

# 指定库文件的安装路径
install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        COMPONENT libraries)