# add the test files
set(ex_list
    test_backends.cxx
    wavplayer.cxx

)

# add check for tests not included in guild install (i.e. lua)

# walk over source files
foreach(srcfile ${ex_list})
    string(REPLACE ".cxx" "" name ${srcfile})
    add_executable(${name} ${srcfile})
    target_link_libraries(${name} 
        vibeaudio
    )

    add_test(${name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name})
endforeach()

