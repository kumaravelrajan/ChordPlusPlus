cmake_minimum_required(VERSION 3.14)
project(dht)

# require C++17
set(CMAKE_CXX_STANDARD 17)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CPM)

enable_testing()

add_subdirectory(src/config)
add_subdirectory(src/dht)
add_subdirectory(src/api)
add_subdirectory(src)