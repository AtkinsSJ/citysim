function(citysim_test source)
    get_filename_component(CITYSIM_TEST_NAME ${source} NAME_WE)
    add_executable(${CITYSIM_TEST_NAME} ${source})
    target_include_directories(${CITYSIM_TEST_NAME} PRIVATE "../src" ${SDL2_INCLUDE_DIRS})
    # TODO: If and when we actually divide things up into libraries, link them!
    target_link_libraries(${CITYSIM_TEST_NAME} PRIVATE ${SDL2_LIBRARIES})
    
    add_test(NAME ${CITYSIM_TEST_NAME}
             COMMAND ${CITYSIM_TEST_NAME})
endfunction()

enable_testing()
citysim_test(TestVariant.cpp)
