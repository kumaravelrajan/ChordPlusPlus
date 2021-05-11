set(BINARY_NAME dht-4)
set(LIBRARY_NAME libDht-4)

add_library(${LIBRARY_NAME} INTERFACE)

target_include_directories(
        ${LIBRARY_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_compile_features(${LIBRARY_NAME} INTERFACE cxx_std_20)

set_target_properties(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${LIBRARY_NAME} INTERFACE lib::api)
target_link_libraries(${LIBRARY_NAME} INTERFACE lib::dht)

add_executable(${BINARY_NAME} dht-4.cpp)
target_link_libraries(${BINARY_NAME} ${LIBRARY_NAME})