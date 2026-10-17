# toolchain for vcpkg
if(WIN32)
    if(DEFINED ENV{VCPKG_ROOT})
        set(_VCPKG_ROOT "$ENV{VCPKG_ROOT}")
    elseif(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
        set(_VCPKG_ROOT "$ENV{VCPKG_INSTALLATION_ROOT}")
    else()
        set(_VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg")
    endif()

    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "${_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE FILEPATH "vcpkg toolchain file" FORCE)
    endif()
endif()

# This is not meant to be included by the top-level.
cmake_minimum_required(VERSION 3.16)
project(VIBE_DEPS C CXX)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Point CMake at any custom modules we may ship
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" "${PROJECT_SOURCE_DIR}/../cmake")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
endif()

# Prefer our bundled versions of dependencies.
if(DEFINED ENV{DEPS_BUILD_DIR})
  set(DEPS_PREFIX "$ENV{DEPS_BUILD_DIR}/../usr" CACHE PATH "Path prefix for finding dependencies")
else()
  set(DEPS_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../.deps/usr" CACHE PATH "Path prefix for finding dependencies")
endif()

# Set install prefix for submodule builds
set(CMAKE_INSTALL_PREFIX "${DEPS_PREFIX}" CACHE PATH "Install path prefix" FORCE)

include(CheckCCompilerFlag)
include(ExternalProject)
include(FindPackageHandleStandardArgs)

# neovim utility functions
include(Deps)

if(WIN32)
    message(STATUS "this is a Windows machine")

    if(NOT DEFINED _VCPKG_ROOT)
        set(_VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg")
    endif()

    set(_VCPKG_INSTALLED "${_VCPKG_ROOT}/installed/x64-windows")

    list(INSERT CMAKE_MODULE_PATH 0 "${_VCPKG_INSTALLED}/share/")
    list(INSERT CMAKE_MODULE_PATH 0 "${_VCPKG_INSTALLED}/share/freetype")
    list(INSERT CMAKE_MODULE_PATH 0 "${_VCPKG_INSTALLED}/lib/pkgconfig")

    list(INSERT CMAKE_PREFIX_PATH 0 "${_VCPKG_INSTALLED}/lib/")
    list(INSERT CMAKE_PREFIX_PATH 0 "${_VCPKG_INSTALLED}/include/")
    list(INSERT CMAKE_PREFIX_PATH 0 "${_VCPKG_INSTALLED}/share/")
    list(INSERT CMAKE_PREFIX_PATH 0 "${_VCPKG_INSTALLED}/share/freetype")
    list(INSERT CMAKE_PREFIX_PATH 0 "${_VCPKG_INSTALLED}/lib/cmake/SDL2")
    list(INSERT CMAKE_PREFIX_PATH 0 "${_VCPKG_INSTALLED}/lib/cmake")
    list(INSERT CMAKE_PREFIX_PATH 0 "${_VCPKG_INSTALLED}/include/freetype")
    list(INSERT CMAKE_PREFIX_PATH 0 "${_VCPKG_INSTALLED}/lib/pkgconfig")

    # @hey: add a runtime output for all libraries
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "" FORCE)

endif(WIN32)
if(UNIX)
  message(STATUS "this is a linux machine")
    list(INSERT CMAKE_PREFIX_PATH 0 "/usr/lib/x86_64-linux-gnu/") # for opengl and sdl
# additional prefix paths
endif(UNIX)

# add dependencies
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/bin/")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/include/")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/lib/")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/share/")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/share/cmake/Corrade")

#-------------------------------------------------------------------------------
# User settings
#-------------------------------------------------------------------------------

set(DEPS_IGNORE_SHA FALSE)

# Options
option(USE_BUNDLED "Use bundled dependencies." ON)
option(BUILD_GUI "build the gui library" ${USE_BUNDLED})
option(USE_BUNDLED_SMG "Use the bundled smg." ${USE_BUNDLED})

if(USE_BUNDLED AND MSVC)
    #option(USE_BUNDLED_GETTEXT "Use the bundled version of gettext." ON)
else()
    #option(USE_BUNDLED_GETTEXT "Use the bundled version of gettext." OFF)
endif()


# linear algebra
option(USE_BUNDLED_SUPERLU "use bundled superlu" ${USE_BUNDLED})
option(USE_BUNDLED_ARMADILLO "use bundled armadillo" ${USE_BUNDLED})

# Get libsoundio from bundled source
#option(USE_BUNDLED_LIBSOUNDIO "use bundled libsoundio" ${USE_BUNDLED})
option(USE_BUNDLED_LIBSOUNDIO "use bundled libsoundio" ON)

# build superlu
if(USE_BUNDLED_SUPERLU)
    include(Buildsuperlu)
endif()

# build armadillo
if(USE_BUNDLED_ARMADILLO)
    include(BuildArmadillo)
endif()

# build jsoncpp
include(Buildjsoncpp)

# for gui
if(USE_BUNDLED_SMG)
    include(BuildSMG)
endif()

if(USE_BUNDLED_LIBSOUNDIO)
    # Build libsoundio as an ExternalProject (keeps deps install in .deps/usr)
    include(BuildLibSoundIo)
endif()

# if using python
if(USE_BUNDLED_PYBIND11)
    include(Buildpybind11)
endif()


