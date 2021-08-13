set(LIBRARY_NAME logging)

set(MODULE_HEADERS centralLogControl.h)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # MSVC cries in the shower if you only give it a header.
    add_library(${LIBRARY_NAME} ${MODULE_HEADERS} dummy.cpp)
else ()
    add_library(${LIBRARY_NAME} ${MODULE_HEADERS})
endif ()
add_library(lib::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME})

target_include_directories(
        ${LIBRARY_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_20)

set_target_properties(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${LIBRARY_NAME} spdlog::spdlog)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${LIBRARY_NAME} PUBLIC -Wall -Wextra -Wconversion -pedantic -Wfatal-errors)
endif ()
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${LIBRARY_NAME} PUBLIC /W3 /WX)
endif ()

