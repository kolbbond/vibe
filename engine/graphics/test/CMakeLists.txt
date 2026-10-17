# vibe-graphics unit tests (CPU-only parts; no GL context)
find_package(Corrade REQUIRED TestSuite)

corrade_add_test(GfxCamera2DTest Camera2DTest.cxx LIBRARIES vibegraphics)
corrade_add_test(GfxBatch2DBuilderTest Batch2DBuilderTest.cxx LIBRARIES vibegraphics)
corrade_add_test(GfxOrbitCameraTest OrbitCameraTest.cxx LIBRARIES vibegraphics)
corrade_add_test(GfxGridMeshTest GridMeshTest.cxx LIBRARIES vibegraphics)
