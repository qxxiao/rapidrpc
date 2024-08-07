FILE(GLOB test_log_src_files  ${project_dir}/src/common/*.cc)
FILE(GLOB test_config_src_files  ${project_dir}/src/common/*.cc)
FILE(GLOB test_eventloop_src_files  ${project_dir}/src/common/*.cc ${project_dir}/src/net/*.cc)
FILE(GLOB test_eventloop_timer_src_files  ${project_dir}/src/common/*.cc ${project_dir}/src/net/*.cc)
FILE(GLOB test_io_thread_src_files  ${project_dir}/src/common/*.cc ${project_dir}/src/net/*.cc)
FILE(GLOB test_io_thread_group_src_files  ${project_dir}/src/common/*.cc ${project_dir}/src/net/*.cc)



add_executable(test_log ${CMAKE_CURRENT_SOURCE_DIR}/test_log.cc ${test_log_src_files})
add_executable(test_config ${CMAKE_CURRENT_SOURCE_DIR}/test_config.cc ${test_config_src_files})
add_executable(test_eventloop ${CMAKE_CURRENT_SOURCE_DIR}/test_eventloop.cc ${test_eventloop_src_files})
add_executable(test_eventloop_timer ${CMAKE_CURRENT_SOURCE_DIR}/test_eventloop_timer.cc ${test_eventloop_timer_src_files})
add_executable(test_io_thread ${CMAKE_CURRENT_SOURCE_DIR}/test_io_thread.cc ${test_io_thread_src_files})
add_executable(test_io_thread_group ${CMAKE_CURRENT_SOURCE_DIR}/test_io_thread_group.cc ${test_io_thread_group_src_files})


find_library(lib_tinyxml NAMES tinyxml PATHS /usr/lib/tinyxml) # 默认不会递归查找
IF(NOT lib_tinyxml)
    message(FATAL_ERROR "tinyxml not found")
ENDIF()
target_link_libraries(test_log PRIVATE "${lib_tinyxml}")
target_link_libraries(test_config PRIVATE "${lib_tinyxml}")
target_link_libraries(test_eventloop PRIVATE "${lib_tinyxml}")
target_link_libraries(test_eventloop_timer PRIVATE "${lib_tinyxml}")
target_link_libraries(test_io_thread PRIVATE "${lib_tinyxml}")
target_link_libraries(test_io_thread_group PRIVATE "${lib_tinyxml}")