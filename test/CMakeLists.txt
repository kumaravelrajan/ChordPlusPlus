macro(my_add_test)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCE_FILES LIBRARIES)
    cmake_parse_arguments(MY_ADD_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    #dump_cmake_variables(".*NAME.*")

    add_executable(test_${MY_ADD_TEST_NAME} ${MY_ADD_TEST_SOURCE_FILES})
    target_compile_features(test_${MY_ADD_TEST_NAME} PUBLIC cxx_std_20)
    target_link_libraries(test_${MY_ADD_TEST_NAME} ${MY_ADD_TEST_LIBRARIES})
    add_test(NAME ${MY_ADD_TEST_NAME} COMMAND test_${MY_ADD_TEST_NAME})
endmacro(my_add_test)

my_add_test(NAME foo SOURCE_FILES foo.cpp)
my_add_test(NAME api SOURCE_FILES test_api.cpp LIBRARIES lib::api)
