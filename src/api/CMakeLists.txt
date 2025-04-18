set(LIBRARY_NAME api)

set(MODULE_HEADERS api.h)

set(MODULE_SOURCES api.cpp request.cpp message_data.cpp connection.cpp)

add_library(${LIBRARY_NAME} ${MODULE_HEADERS} ${MODULE_SOURCES})
add_library(lib::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME})

target_include_directories(
	${LIBRARY_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_20)

set_target_properties(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${LIBRARY_NAME} lib::logging)
target_link_libraries(${LIBRARY_NAME} lib::config)
target_link_libraries(${LIBRARY_NAME} lib::util)
target_link_libraries(${LIBRARY_NAME} Threads::Threads)
target_link_libraries(${LIBRARY_NAME} asio)

add_executable(apiMain main.cpp)
target_link_libraries(apiMain ${LIBRARY_NAME})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(apiMain PUBLIC -Wall -Wextra -Wconversion -pedantic -Wfatal-errors)
    target_compile_options(${LIBRARY_NAME} PUBLIC -Wall -Wextra -Wconversion -pedantic -Wfatal-errors)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(apiMain PUBLIC /W3 /WX)
    target_compile_options(${LIBRARY_NAME} PUBLIC /W3 /WX)
endif()

