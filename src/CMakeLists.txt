set(BINARY_NAME dht-4)
set(LIBRARY_NAME libDht-4)

add_library(${LIBRARY_NAME} INTERFACE)

target_include_directories(
        ${LIBRARY_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_link_libraries(${LIBRARY_NAME} INTERFACE cxxopts)
target_link_libraries(${LIBRARY_NAME} INTERFACE lib::config)
target_link_libraries(${LIBRARY_NAME} INTERFACE lib::entry)

add_executable(${BINARY_NAME} dht-4.cpp)
target_link_libraries(${BINARY_NAME} ${LIBRARY_NAME})

install(TARGETS ${BINARY_NAME} DESTINATION bin)