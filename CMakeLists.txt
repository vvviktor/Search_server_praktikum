cmake_minimum_required(VERSION 3.10)

project(search_server_praktikum)
set(CMAKE_CXX_STANDARD 17)

set(SEARCH_SERVER_FILES search_server.h search_server.cpp concurrent_map.h document.h document.cpp
    string_processing.h string_processing.cpp)
set(READ_INPUT_FILES read_input_functions.h read_input_functions.cpp)
set(PROCESS_QUERIES_FILES process_queries.h process_queries.cpp)
set(REMOVE_DUPLICATES_FILES remove_duplicates.h remove_duplicates.cpp)
set(REQUEST_QUEUE_FILES request_queue.h request_queue.cpp)
set(PAGINATOR paginator.h)
set(TESTS_FILES my_assert.h my_assert.cpp log_duration.h ss_tests.h ss_tests.cpp)
set(MAIN main.cpp)

add_executable(search_server ${MAIN} ${SEARCH_SERVER_FILES} ${READ_INPUT_FILES} ${PROCESS_QUERIES_FILES}
               ${REMOVE_DUPLICATES_FILES} ${REQUEST_QUEUE_FILES} ${PAGINATOR} ${TESTS_FILES})