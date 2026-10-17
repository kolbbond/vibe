# vibe-fluid: 2D fluid simulation, audio -> fluid emitters, presets (no GL)
# installed and exported through the Vibe package (cmake/VibeConfig.cmake.in)

# Fluid: solver, emitters, band tracker; Magnum Math value types only.
# PIC because vibe-graphics (a shared library) links it for FluidRenderer.
add_library(vibefluid STATIC
    src/CpuStableFluid.cxx
    src/Emitters.cxx
    src/BandTracker.cxx
)
add_library(Vibe::Fluid ALIAS vibefluid)

# expose headers as vibe/fluid/X.hxx in the build tree (matches the other engine libs)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/vibe)
if(NOT EXISTS ${CMAKE_BINARY_DIR}/include/vibe/fluid)
    file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include/vibe/fluid RESULT _vibe_link SYMBOLIC)
    if(NOT _vibe_link EQUAL 0)
        # no symlink permission (e.g. Windows without developer mode): copy; refreshed only on a clean configure
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_BINARY_DIR}/include/vibe/fluid)
    endif()
endif()

target_include_directories(vibefluid PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(vibefluid PUBLIC Magnum::Magnum)
set_target_properties(vibefluid PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(vibefluid PRIVATE "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic;-Wshadow>")

# FluidPresets: preset JSON load/save. jsoncpp from .deps is not PIC, so this
# stays out of vibe-graphics (a shared library); only executables link it.
if(NOT TARGET JsonCpp::JsonCpp)
    find_package(jsoncpp CONFIG REQUIRED)
endif()
add_library(vibefluidpresets STATIC src/Preset.cxx)
add_library(Vibe::FluidPresets ALIAS vibefluidpresets)
target_link_libraries(vibefluidpresets PUBLIC vibefluid PRIVATE JsonCpp::JsonCpp)
target_compile_options(vibefluidpresets PRIVATE "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic;-Wshadow>")

## INSTALLATION
include(GNUInstallDirs)
set_target_properties(vibefluid PROPERTIES EXPORT_NAME Fluid)
set_target_properties(vibefluidpresets PROPERTIES EXPORT_NAME FluidPresets)
install(TARGETS vibefluid vibefluidpresets
    EXPORT vibefluid-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/vibe
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vibe/fluid)
install(EXPORT vibefluid-targets
    FILE VibeFluidTargets.cmake
    NAMESPACE Vibe::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Vibe
)

if(ENABLE_TESTING)
    add_subdirectory(test)
endif()
