## Project setup
if(WIN32)
    if(DEFINED ENV{VCPKG_ROOT})
        set(_VCPKG_ROOT "$ENV{VCPKG_ROOT}")
    elseif(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
        set(_VCPKG_ROOT "$ENV{VCPKG_INSTALLATION_ROOT}")
    endif()

    if(DEFINED _VCPKG_ROOT AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "${_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE FILEPATH "vcpkg toolchain file" FORCE)
    endif()
endif()

cmake_minimum_required(VERSION 3.24)  # CMAKE_FIND_PACKAGE_TARGETS_GLOBAL

# Default build type: RelWithDebInfo (optimized + debug symbols)
# MUST be set before project() so vcpkg toolchain picks correct libraries
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
    message(STATUS "Build type not specified, defaulting to RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COLOR_DIAGNOSTICS ON)

# Read version from VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VIBE_VERSION)
string(STRIP "${VIBE_VERSION}" VIBE_VERSION)

# our project starts
project(vibe VERSION ${VIBE_VERSION} LANGUAGES C CXX)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Keep asserts enabled even in Release/RelWithDebInfo builds
string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

message(STATUS "starting cmake generation: " ${PROJECT_NAME})
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# as a subproject (add_subdirectory from a game/app repo) build the engine libs only
if(NOT PROJECT_IS_TOP_LEVEL)
    set(ENABLE_TESTING OFF)
endif()

# switchboard
option(ENABLE_TESTING "build unit/system tests" ON)
option(ENABLE_INSTALL "INSTALL" ON)

# module path
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "" FORCE)

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

# DEPS_PREFIX, module/prefix paths and RPATH (also included by consumers)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/VibeDeps.cmake)

message(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})
message(STATUS "CMAKE_PREFIX_PATH: " ${CMAKE_PREFIX_PATH})


# find dependencies
message(STATUS "find packages")

# find libraries
#find_package(smg REQUIRED)
#find_package(Lua 5.4 REQUIRED)

# filesystem library
#set(Boost_NO_WARN_NEW_VERSIONS 1)
find_package(Boost 1.80.0 REQUIRED COMPONENTS filesystem iostreams)

# debug lua find
if(LUA_FOUND)
    message(STATUS "LUA found")
    message(STATUS "LUA library: " "${LUA_LIBRARIES}")
endif()

find_package(Armadillo CONFIG REQUIRED)
set(IMGUI_DIR "${DEPS_PREFIX}/include/MagnumExternal/ImGui")
find_package(Corrade CONFIG REQUIRED)
find_package(Magnum CONFIG REQUIRED)
find_package(MagnumIntegration CONFIG REQUIRED ImGui)
find_package(smg CONFIG REQUIRED)

if(ENABLE_TESTING)
    enable_testing()
endif()

# engine libraries (in-tree)
# ncurses UI was off in the old cmake.vibe superbuild; keep that default
option(ENABLE_CURSES "build vibe-graphics with ncurses" OFF)
add_subdirectory(engine/common)
add_subdirectory(engine/audio)
add_subdirectory(engine/fluid)
add_subdirectory(engine/graphics)
add_subdirectory(engine/game)

# umbrella package: find_package(Vibe) -> every Vibe:: target, plus VibeManifest.cmake
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VibeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/VibeConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Vibe
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/VibeConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/VibeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/VibeConfigVersion.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VibeManifest.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckGamesJson.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindTCLAP.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Vibe
)

# deps shared libraries next to the engine libs (also when a consumer installs or packages)
if(WIN32)
    # Dependency DLLs
    install(FILES "${DEPS_PREFIX}/lib/soundio.dll" DESTINATION bin)
    install(DIRECTORY "${DEPS_PREFIX}/bin/" DESTINATION bin FILES_MATCHING PATTERN "*.dll")
    # Magnum plugins (importers, fonts, etc.)
    install(DIRECTORY "${DEPS_PREFIX}/bin/magnum" DESTINATION bin)
    # vcpkg DLLs (SDL2, FFmpeg, etc.)
    if(DEFINED ENV{VCPKG_ROOT})
        file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" _VCPKG_ROOT_CMAKE)
    else()
        file(TO_CMAKE_PATH "${_VCPKG_ROOT}" _VCPKG_ROOT_CMAKE)
    endif()
    set(_VCPKG_BIN "${_VCPKG_ROOT_CMAKE}/installed/x64-windows/bin")
    install(DIRECTORY "${_VCPKG_BIN}/" DESTINATION bin FILES_MATCHING PATTERN "*.dll")
else()
    # Linux shared libraries
    install(DIRECTORY "${DEPS_PREFIX}/lib/" DESTINATION lib/vibe FILES_MATCHING PATTERN "*.so*" PATTERN "*.conf")
    # executables RUNPATH $ORIGIN/../lib/vibe; smg installs into lib/smg, so flatten it (as make install does)
    install(DIRECTORY "${DEPS_PREFIX}/lib/smg/" DESTINATION lib/vibe FILES_MATCHING PATTERN "*.so*")
endif()

if(NOT PROJECT_IS_TOP_LEVEL)
    return()
endif()

# add main target to link against and force dependencies
add_custom_target(vibe_all ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Building all Vibe components..."
    COMMENT "Building all Vibe components"
)

# run this after building
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
else()
    set(PYTHON_EXECUTABLE python)
endif()
#add_custom_command(
    #TARGET vibe_all POST_BUILD
    #COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/merge_json.py"
    #COMMENT " ... Running final post-build script ... "
    #)
# run this after building
add_custom_command(
    TARGET vibe_all POST_BUILD
    COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/merge_json.py"
    COMMENT "Running final post-build script..."
)

# Windows: third-party DLLs (and Magnum plugins) next to the executables so they run from the
# build tree. Copied at configure time: copying during the build raced the linkers writing to the
# same bin/ ("permission denied" on avcodec-*.dll). The engine's own DLLs are built into bin/.
if(WIN32)
    if(DEFINED ENV{VCPKG_ROOT})
        file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" _VCPKG_ROOT_PATH)
    else()
        file(TO_CMAKE_PATH "${_VCPKG_ROOT}" _VCPKG_ROOT_PATH)
    endif()
    set(VCPKG_BIN_DIR "${_VCPKG_ROOT_PATH}/installed/x64-windows/bin")
    foreach(dir "${DEPS_PREFIX}/bin" "${DEPS_PREFIX}/lib" "${VCPKG_BIN_DIR}")
        if(NOT IS_DIRECTORY "${dir}")
            continue()
        endif()
        if(dir STREQUAL "${DEPS_PREFIX}/lib")
            file(GLOB _runtime "${dir}/*.dll") # soundio.dll; the rest of lib/ is import libs and cmake files
            file(COPY ${_runtime} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        else()
            file(GLOB_RECURSE _runtime "${dir}/*.dll" "${dir}/*.conf")
            file(COPY "${dir}/" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" FILES_MATCHING PATTERN "*.dll" PATTERN "*.conf")
        endif()
        # a rebuilt .deps or a new vcpkg package re-runs configure, which copies again
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_runtime})
    endforeach()
endif()

# test @hey: move to sublibraries
if(ENABLE_TESTING)
    message(STATUS "testing enabled")
    add_subdirectory(tests)
endif()
