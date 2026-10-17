find_package(Corrade REQUIRED TestSuite)

corrade_add_test(FluidSolverTest FluidSolverTest.cxx LIBRARIES Vibe::Fluid)
corrade_add_test(FluidEmitterTest EmitterTest.cxx LIBRARIES Vibe::Fluid)
corrade_add_test(FluidBandTrackerTest BandTrackerTest.cxx LIBRARIES Vibe::Fluid)
corrade_add_test(FluidPresetTest PresetTest.cxx LIBRARIES Vibe::FluidPresets)
target_compile_definitions(FluidPresetTest PRIVATE VVAVVIZ_PRESETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/presets")
corrade_add_test(FluidViewTest ViewTest.cxx LIBRARIES Vibe::Fluid)
