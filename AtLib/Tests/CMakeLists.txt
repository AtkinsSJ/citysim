find_package(SDL2 REQUIRED)

function(atlib_test source)
    get_filename_component(ATLIB_TEST_NAME ${source} NAME_WE)
    add_executable(${ATLIB_TEST_NAME} ${source})
    target_include_directories(${ATLIB_TEST_NAME} PRIVATE "../" ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${ATLIB_TEST_NAME} PRIVATE Util ${SDL2_LIBRARIES})

    target_compile_definitions(${ATLIB_TEST_NAME} PRIVATE
        BUILD_DEBUG=0
    )
    
    add_test(NAME ${ATLIB_TEST_NAME}
             COMMAND ${ATLIB_TEST_NAME})
endfunction()

enable_testing()
atlib_test(TestFunction.cpp)
atlib_test(TestHashTable.cpp)
atlib_test(TestOwnedPtr.cpp)
atlib_test(TestVariant.cpp)
