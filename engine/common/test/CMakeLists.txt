# add the test files
set(test_list
)

# add check for tests not included in guild install (i.e. lua)

# walk over source files
foreach(srcfile ${test_list})
    string(REPLACE ".cxx" "" name ${srcfile})
    add_executable(${name} ${srcfile})
    target_link_libraries(${name} 
        vibegraphics
    )

    add_test(${name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name})
endforeach()
