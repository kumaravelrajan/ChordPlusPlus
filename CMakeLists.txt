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
        VERSION 1-18-2
        GITHUB_REPOSITORY chriskohlhoff/asio
        GIT_TAG asio-1-18-2
)
if (asio_ADDED)
    add_library(asio INTERFACE)

    target_include_directories(asio
            INTERFACE ${asio_SOURCE_DIR}/asio/include
            )

    target_compile_definitions(asio
            INTERFACE
            ASIO_STANDALONE
            )

    target_link_libraries(asio
            INTERFACE
            Threads::Threads
            )

    if (WIN32)
        # macro see @ https://stackoverflow.com/a/40217291/1746503
        macro(get_win32_winnt version)
            if (CMAKE_SYSTEM_VERSION)
                set(ver ${CMAKE_SYSTEM_VERSION})
                string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
                string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
                if ("${verMajor}" MATCHES "10")
                    set(verMajor "A")
                    string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
                endif ("${verMajor}" MATCHES "10")
                string(REPLACE "." "" ver ${ver})
                string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
                set(${version} "0x${ver}")
            endif ()
        endmacro()

        if (NOT DEFINED _WIN32_WINNT)
            get_win32_winnt(ver)
            set(_WIN32_WINNT ${ver})
        endif ()

        message(STATUS "Set _WIN32_WINNET=${_WIN32_WINNT}")

        target_compile_definitions(asio
                INTERFACE
                _WIN32_WINNT=${_WIN32_WINNT}
                WIN32_LEAN_AND_MEAN
                )
    endif ()
endif ()

# pull cxxopts using cpm:
CPMAddPackage(
        NAME cxxopts
        VERSION 2.2.1
        GITHUB_REPOSITORY jarro2783/cxxopts
        OPTIONS "CXXOPTS_BUILD_TESTS OFF"
)

# pull inipp using cpm:
CPMAddPackage(
        NAME inipp
        VERSION 1.0.13
        GITHUB_REPOSITORY MixusMinimax/inipp
        GIT_TAG 1.0.13
        OPTIONS "INIPP_ENABLE_TESTING OFF"
)
if (inipp_ADDED)
    add_library(inipp INTERFACE IMPORTED)
    target_include_directories(inipp INTERFACE ${inipp_SOURCE_DIR}/inipp)
endif ()


add_subdirectory(src)
add_subdirectory(src/config)
add_subdirectory(src/dht)
add_subdirectory(src/api)
add_subdirectory(src/util)

add_subdirectory(lib/cpp_unit_test)

add_subdirectory(test)