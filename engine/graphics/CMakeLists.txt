## Project setup
cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COLOR_DIAGNOSTICS ON)

# Read version from VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../../VERSION" VIBE_VERSION)
string(STRIP "${VIBE_VERSION}" VIBE_VERSION)

# our project starts
project(Vibe-Graphics VERSION ${VIBE_VERSION} LANGUAGES C CXX)

# output paths
#@hey: need to check for whether this is a superbuild
#if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY AND NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY AND NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
# output paths
if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin CACHE PATH "" FORCE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib CACHE PATH "" FORCE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib CACHE PATH "" FORCE)
endif()
message("CMAKE_RUNTIME_OUTPUT_DIRECTORY: " ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    #endif()

message(STATUS "starting cmake generation: " ${PROJECT_NAME})

# switchboard
option(ENABLE_TESTING "build unit/system tests" ON)
option(ENABLE_INSTALL "install library" ON)
option(ENABLE_CURSES "build with ncurses" ON)
# SceneGraph disabled on Windows - causes GUI freeze issues
if(WIN32)
    option(ENABLE_SCENEGRAPH "build with Magnum SceneGraph support" OFF)
else()
    option(ENABLE_SCENEGRAPH "build with Magnum SceneGraph support" ON)
endif()
#option(USE_ARMADILLO "building with armadillo" ON) #include with smg

# module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake ${CMAKE_MODULE_PATH})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# check system
message(STATUS "check system architecture")
if(WIN32)
    message(STATUS "this is a windows machine")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif(WIN32)
if(UNIX)
    message(STATUS "this is a linux machine")
endif(UNIX)

# ensure out of core build
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds not allowed.\nPlease make a new directory (called a build directory) and run CMake from there.\n")
endif()

message(STATUS "find packages")

# find libraries
if(NOT TARGET Vibe::Common)
    find_package(VibeCommon REQUIRED)
endif()
if(NOT TARGET Vibe::Audio)
    find_package(VibeAudio REQUIRED)
endif()

# curses not available on windows
if(ENABLE_CURSES)
    find_package(Curses REQUIRED)
    if(NOT TARGET Curses::Curses)
        # create target
        add_library(Curses::Curses INTERFACE IMPORTED GLOBAL)
        set_target_properties(Curses::Curses PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${CURSES_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${CURSES_LIBRARIES}")
    endif()
else()
    message(STATUS "curses not enabled, not building with ncurses")
endif()

# smg is required (brings in Magnum SceneGraph)
find_package(smg REQUIRED)

# Boost filesystem for path handling
find_package(Boost 1.80.0 REQUIRED COMPONENTS filesystem)

#if(USE_ARMADILLO)
    #find_package(Armadillo REQUIRED)
#endif()



##################################################
# Create libraries

# add source list
# ls *.cxx >> CMakeLists.txt
set(source_list
    src/FFTView.cxx
    src/SHBlobShader.cxx
    src/SHBlobView.cxx
    src/SHBlob3DView.cxx
    src/SpectralView.cxx
    src/WaterfallShader.cxx
    src/WaterfallRibbonShader.cxx
    src/WaterfallView.cxx
    src/Hitbox.cxx
    src/Mesher.cxx
    src/MidiParser.cxx
    src/Plotter.cxx
    src/Camera2D.cxx
    src/Batch2DBuilder.cxx
    src/Batch2D.cxx
    src/FluidShader.cxx
    src/FluidLookShader.cxx
    src/Fluid3DShader.cxx
    src/FluidRenderer.cxx
    src/OrbitCamera.cxx
    src/GridMesh.cxx
)

# SceneGraph-dependent files
if(ENABLE_SCENEGRAPH)
    list(APPEND source_list
        src/Callbacks.cxx
        src/ColoredDrawable.cxx
        src/Entity.cxx
        src/Events.cxx
        src/GuiMain.cxx
        src/MeshSwapAnimator.cxx
        src/ObjData.cxx
        src/TexturedDrawable.cxx
    )
endif()

if(ENABLE_CURSES)
    list(APPEND source_list
        src/NCurses.cxx
    )
endif()

# add library ... ? makes a library
add_library(vibegraphics SHARED ${source_list})
add_library(Vibe::Graphics ALIAS vibegraphics)

# expose headers as vibe/graphics/X.hxx in the build tree (matches install layout)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/vibe)
if(NOT EXISTS ${CMAKE_BINARY_DIR}/include/vibe/graphics)
    file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include/vibe/graphics RESULT _vibe_link SYMBOLIC)
    if(NOT _vibe_link EQUAL 0)
        # no symlink permission (e.g. Windows without developer mode): copy; refreshed only on a clean configure
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_BINARY_DIR}/include/vibe/graphics)
    endif()
endif()

# include directories 
# as interfaces to add includes to anything that adds these libraries
target_include_directories(vibegraphics 
    PUBLIC 
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        )

if(ENABLE_CURSES)
    target_include_directories(vibegraphics 
        PUBLIC 
    ${CURSES_INCLUDE_DIR}
    )
endif()

set_target_properties(vibegraphics PROPERTIES
    LINKER_LANGUAGE CXX
    # Set RPATH for finding dependencies at runtime
    BUILD_RPATH "${DEPS_PREFIX}/lib"
    INSTALL_RPATH "$ORIGIN;$ORIGIN/smg;$ORIGIN/../lib;$ORIGIN/../lib/smg;$ORIGIN/../../.deps/usr/lib"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

# enable warnings
message(STATUS "compiler id: " ${CXX_COMPILER_ID})
target_compile_options(vibegraphics PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wpedantic -Wall -Wextra>
        #-W>
        )

# link to library
target_link_libraries(vibegraphics PUBLIC
    # armadillo
    Vibe::Common
    Vibe::Audio
    smg::smg
    Boost::filesystem
 )

# FluidRenderer draws a vibe-fluid solver (static PIC lib, kept out of the export interface)
target_link_libraries(vibegraphics PRIVATE Vibe::Fluid)

# SceneGraph support
if(ENABLE_SCENEGRAPH)
    target_compile_definitions(vibegraphics PUBLIC ENABLE_SCENEGRAPH)
endif()

if(ENABLE_CURSES)
      target_link_libraries(vibegraphics PUBLIC
          Curses::Curses
      )
endif()

message(STATUS "building in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

message(STATUS "setup tests")
# testing options
if(ENABLE_TESTING)
    #include(CTest)
    enable_testing()

    # run cmakelists in sub directory
    add_subdirectory(test)
endif()

  message(STATUS "install vibe-graphics")


# Version header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hh.in 
    ${CMAKE_CURRENT_BINARY_DIR}/include/version.hh)


## INSTALLATION
# get install directories
    include(GNUInstallDirs)
    set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/vibegraphics)

    install(TARGETS vibegraphics
        EXPORT vibegraphics-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/vibe/
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/vibe/
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

# This is required so that the exported target has the name vibe and not vibe
set_target_properties(vibegraphics PROPERTIES EXPORT_NAME Graphics)

# install includes
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vibe/graphics)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/version.hh DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vibe/graphics)

# Export the targets to a script
    install(EXPORT vibegraphics-targets
        FILE VibeGraphicsTargets.cmake
        NAMESPACE Vibe::
        DESTINATION ${INSTALL_CONFIGDIR}
    )

# Create a ConfigVersion.cmake file
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/VibeGraphicsConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/VibeGraphicsConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/VibeGraphicsConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
    )

# Install the config, configversion and custom find modules
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/VibeGraphicsConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/VibeGraphicsConfigVersion.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
    )

## EXPORT
# Exporting from the build tree

    export(EXPORT vibegraphics-targets
        FILE ${CMAKE_CURRENT_BINARY_DIR}/VibeGraphicsTargets.cmake
        NAMESPACE Vibe::)

# Register package in user's package registry
    export(PACKAGE Vibe)


#####################
# Setup install paths
#####################

    include(GNUInstallDirs)

# Set RPATHS
    SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")

  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# add additional examples
#add_subdirectory(examples)
