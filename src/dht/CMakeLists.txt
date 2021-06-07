set(LIBRARY_NAME dht)

set(MODULE_HEADERS dht.h)

set(MODULE_SOURCES dht.cpp NodeInformation.cpp NodeInformation.h)

capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS schemas/person.capnp)

add_library(${LIBRARY_NAME} ${MODULE_HEADERS} ${MODULE_SOURCES} ${CAPNP_SRCS} ${CAPNP_HDRS})
add_library(lib::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME})

target_include_directories(
	${LIBRARY_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_20)

set_target_properties(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${LIBRARY_NAME} lib::api)
target_link_libraries(${LIBRARY_NAME} CapnProto::capnp-rpc)
target_link_libraries(${LIBRARY_NAME} ssl)
target_include_directories(${LIBRARY_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/schemas)

add_executable(dhtMain main.cpp)
target_link_libraries(dhtMain ${LIBRARY_NAME})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(dhtMain PUBLIC -Wall -Wextra -Wconversion -pedantic -Wfatal-errors)
    target_compile_options(${LIBRARY_NAME} PUBLIC -Wall -Wextra -Wconversion -pedantic -Wfatal-errors)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(dhtMain PUBLIC /W3 /WX)
    target_compile_options(${LIBRARY_NAME} PUBLIC /W3 /WX)
endif()

