## Project setup
cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COLOR_DIAGNOSTICS ON)

# Read version from VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../../VERSION" VIBE_VERSION)
string(STRIP "${VIBE_VERSION}" VIBE_VERSION)

# our project starts
project(Vibe-Audio VERSION ${VIBE_VERSION} LANGUAGES C CXX)

# output paths
#@hey: need to check for whether this is a superbuild
#if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY AND NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY AND NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
# output paths
if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin CACHE PATH "" FORCE)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib CACHE PATH "" FORCE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib CACHE PATH "" FORCE)
endif()

    #endif()
message("CMAKE_RUNTIME_OUTPUT_DIRECTORY: " ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
message("CMAKE_LIBRARY_OUTPUT_DIRECTORY: " ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
message("CMAKE_ARCHIVE_OUTPUT_DIRECTORY: " ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

message(STATUS "starting cmake generation: " ${PROJECT_NAME})

# switchboard
option(ENABLE_TESTING "build unit/system tests" ON)

# module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake ${CMAKE_MODULE_PATH})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_BUILD_TYPE Debug)  # Don't override - inherit from parent

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

find_package(SoundIo REQUIRED)
if(NOT TARGET SoundIo::SoundIo)
    # create target
    add_library(SoundIo::SoundIo INTERFACE IMPORTED GLOBAL)
    set_target_properties(SoundIo::SoundIo PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SOUNDIO_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${SOUNDIO_LIBRARY}")
endif()
find_package(FFTW REQUIRED)

# FFmpeg for decoding various audio formats
# Find pkgconf/pkg-config (on Windows, try vcpkg's pkgconf)
if(WIN32 AND NOT DEFINED PKG_CONFIG_EXECUTABLE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(_pkgconf_hint "$ENV{VCPKG_ROOT}/installed/x64-windows/tools/pkgconf/pkgconf.exe")
        if(EXISTS "${_pkgconf_hint}")
            set(PKG_CONFIG_EXECUTABLE "${_pkgconf_hint}")
        endif()
        unset(_pkgconf_hint)
    endif()
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
    libavformat
    libavcodec
    libswresample
    libavutil
)

# Create unified interface target
if(NOT TARGET FFmpeg::FFmpeg)
    add_library(FFmpeg::FFmpeg INTERFACE IMPORTED GLOBAL)
    target_include_directories(FFmpeg::FFmpeg INTERFACE ${FFMPEG_INCLUDE_DIRS})
    target_link_directories(FFmpeg::FFmpeg INTERFACE ${FFMPEG_LIBRARY_DIRS})
    target_link_libraries(FFmpeg::FFmpeg INTERFACE ${FFMPEG_LIBRARIES})
endif()

# tclap command line input parser
find_package(TCLAP REQUIRED)

##################################################
# Create libraries

# add source list
# ls *.cxx >> CMakeLists.txt
set(source_list
#       src/MidiParser.cxx
        src/NoiseMaker.cxx
        src/Pitch.cxx
        src/LibSoundIo.cxx
        src/AudioData.cxx
        src/AudioBuffer.cxx
        src/WavData.cxx
        src/StreamData.cxx
        src/WavPlayer.cxx
        src/FFT.cxx
        src/FFmpegAudioData.cxx
        src/AudioCapture.cxx
        src/SpectralAnalysis.cxx
        src/MusicPlayer.cxx
)

# Windows-only: WASAPI loopback capture
if(WIN32)
    list(APPEND source_list src/WasapiLoopback.cxx)
endif()

# add library ... ? makes a library
add_library(vibeaudio SHARED ${source_list})
add_library(Vibe::Audio ALIAS vibeaudio)
# installed next to its deps in <prefix>/lib/vibe (smg in lib/vibe/smg)
set_target_properties(vibeaudio PROPERTIES INSTALL_RPATH "$ORIGIN;$ORIGIN/smg")

# expose headers as vibe/audio/X.hxx in the build tree (matches install layout)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/vibe)
if(NOT EXISTS ${CMAKE_BINARY_DIR}/include/vibe/audio)
    file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include/vibe/audio RESULT _vibe_link SYMBOLIC)
    if(NOT _vibe_link EQUAL 0)
        # no symlink permission (e.g. Windows without developer mode): copy; refreshed only on a clean configure
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_BINARY_DIR}/include/vibe/audio)
    endif()
endif()

# include directories
# as interfaces to add includes to anything that adds these libraries
target_include_directories(vibeaudio
    PUBLIC
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        )

set_target_properties(vibeaudio PROPERTIES
    LINKER_LANGUAGE CXX
)

# enable warnings
message(STATUS "compiler id: " ${CXX_COMPILER_ID})
target_compile_options(vibeaudio PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wpedantic -Wall -Wextra>
        #-W>
        )

# link to library
target_link_libraries(vibeaudio PUBLIC
    Vibe::Common
    Arma::Armadillo
    SoundIo::SoundIo
    FFTW::Double
 )

# Link FFmpeg libraries (private since only used internally)
target_link_libraries(vibeaudio PRIVATE FFmpeg::FFmpeg)


message(STATUS "building in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

message(STATUS "setup tests")
# testing options
if(ENABLE_TESTING)
    #include(CTest)
    enable_testing()

    # run cmakelists in sub directory
    add_subdirectory(test)
endif()

message(STATUS "install vibe-audio")

# Version header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hh.in 
    ${CMAKE_CURRENT_BINARY_DIR}/include/version.hh
)

## INSTALLATION
# get install directories
include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/vibeaudio)

install(TARGETS vibeaudio
    EXPORT vibeaudio-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/vibe
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/vibe
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# This is required so that the exported target has the name vibe and not vibe
set_target_properties(vibeaudio PROPERTIES EXPORT_NAME Audio)

# install includes
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vibe/audio)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/version.hh DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vibe/audio)

# Export the targets to a script
    install(EXPORT vibeaudio-targets
        FILE VibeAudioTargets.cmake
        NAMESPACE Vibe::
        DESTINATION ${INSTALL_CONFIGDIR}
    )

# Create a ConfigVersion.cmake file
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/VibeAudioConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/VibeAudioConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/VibeAudioConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
    )

# Install the config, configversion and custom find modules
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/VibeAudioConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/VibeAudioConfigVersion.cmake
        ${CMAKE_CURRENT_LIST_DIR}/cmake/FindFFTW.cmake
        ${CMAKE_CURRENT_LIST_DIR}/cmake/FindSoundIo.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
    )

## EXPORT
# Exporting from the build tree
configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/FindFFTW.cmake
	${CMAKE_CURRENT_BINARY_DIR}/FindFindFFTW.cmake COPYONLY)

    export(EXPORT vibeaudio-targets
        FILE ${CMAKE_CURRENT_BINARY_DIR}/VibeAudioTargets.cmake
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


# add additional examples
#add_subdirectory(examples)

## install examples
#install (TARGETS wavplayer RUNTIME DESTINATION bin)
