cmake_minimum_required(VERSION 3.14)
project(dht)

# require C++17
set(CMAKE_CXX_STANDARD 17)

if ("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif ()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CPM)
include(Tools)
include(CTest)
find_package(Threads REQUIRED)

# pull asio using cpm:
CPMAddPackage(
        NAME asio
        GITHUB_REPOSITORY chriskohlhoff/asio
        GIT_TAG asio-1-18-2
)
if (asio_ADDED)
    add_library(asio INTERFACE IMPORTED)
    target_include_directories(asio INTERFACE "${asio_SOURCE_DIR}/asio/include")
else ()
    message(FATAL_ERROR "asio not added!")
endif ()

# pull cxxopts using cpm:
CPMAddPackage("gh:jarro2783/cxxopts@2.2.1")

# pull inih using cpm:
CPMAddPackage(
        NAME inih
        GITHUB_REPOSITORY benhoyt/inih
        GIT_TAG r53
)
if (inih_ADDED)
    add_library(inih INTERFACE IMPORTED)
    target_include_directories(inih INTERFACE "${inih_SOURCE_DIR}/cpp")
else ()
    message(FATAL_ERROR "inih not added!")
endif ()


add_subdirectory(src)
add_subdirectory(src/config)
add_subdirectory(src/dht)
add_subdirectory(src/api)
add_subdirectory(src/util)

add_subdirectory(test)