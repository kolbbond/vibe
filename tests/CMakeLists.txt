# tests cmakelists

message(STATUS "testing ")

# add the test files
set(test_list
    test_armadillo.cxx
    test_hitbox.cxx
    test_vecnorm.cxx
    test_input_devices.cxx
    test_spherical_harmonics.cxx
)


# walk over source files
foreach(srcfile ${test_list})
  string(REPLACE ".cxx" "" name ${srcfile})
  add_executable(${name} ${srcfile})
  target_link_libraries(${name} 
    Arma::Armadillo
    Vibe::Common
    Vibe::Audio
    Vibe::Graphics
  )

  add_test(${name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name})
endforeach()

# SpectralAnalysis test (helpers now live in vibe-audio library)
add_executable(test_audio_pipeline test_audio_pipeline.cxx)
target_link_libraries(test_audio_pipeline
    Arma::Armadillo
    Vibe::Common
    Vibe::Audio
)
add_test(test_audio_pipeline ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_audio_pipeline)

message(STATUS "end of test creation")
