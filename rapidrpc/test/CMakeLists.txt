FILE(GLOB test_log_src_files  ${project_dir}/src/common/*.cc)
FILE(GLOB test_config_src_files  ${project_dir}/src/common/*.cc)



add_executable(test_log ${CMAKE_CURRENT_SOURCE_DIR}/test_log.cc ${test_log_src_files})
add_executable(test_config ${CMAKE_CURRENT_SOURCE_DIR}/test_config.cc ${test_config_src_files})


find_library(lib_tinyxml NAMES tinyxml) # PATHS /usr/lib/tinyxml
# message(STATUS "lib_tinyxml: ${lib_tinyxml}")
target_link_libraries(test_log PRIVATE "${lib_tinyxml}")
target_link_libraries(test_config PRIVATE "${lib_tinyxml}")