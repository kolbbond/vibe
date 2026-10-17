## Project setup
cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COLOR_DIAGNOSTICS ON)

# Read version from VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../../VERSION" VIBE_VERSION)
string(STRIP "${VIBE_VERSION}" VIBE_VERSION)

# our project starts
project(Vibe-Common VERSION ${VIBE_VERSION} LANGUAGES C CXX)

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
find_package(Armadillo CONFIG REQUIRED)
if(NOT TARGET Arma::Armadillo)
    add_library(Arma::Armadillo INTERFACE IMPORTED GLOBAL)
    set_target_properties(Arma::Armadillo PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ARMADILLO_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${ARMADILLO_LIBRARIES}"
    )
    message(STATUS "Armadillo library: ${ARMADILLO_LIBRARY}")
    message(STATUS "Armadillo includes: ${ARMADILLO_INCLUDE_DIRS}")

    # disable opernmp acceleration
    # target_compile_definitions(Arma::Armadillo INTERFACE -DARMA_DONT_USE_OPENMP)

    # set (n)debug flags for Armadilo
    if(CMAKE_BUILD_TYPE STREQUAL Release)
        message(STATUS "Using Armadillo no-debug flags (conformance check disabled!)")
        target_compile_definitions(Arma::Armadillo INTERFACE -DNDEBUG -DARMA_NO_DEBUG -DARMA_DONT_CHECK_CONFORMANCE -DARMA_WARN_LEVEL=1)
    else()
        message(STATUS "Using Armadillo in debug mode")
    endif()

    # disable armadillo wrapper (for using custom BLAS)
    #if(BLAS_FOUND AND LAPACK_FOUND)
        # report to user
        #    message(STATUS "Disabling Armadillo BLAS wrapper.")
        #target_compile_definitions(Arma::Armadillo INTERFACE -DARMA_DONT_USE_WRAPPER -DARMA_USE_BLAS)
        #endif()

    # disable superlu if not available
    if(NOT SUPERLU_FOUND)
        message(STATUS "Disabling Armadillo SuperLU")
        target_compile_definitions(Arma::Armadillo INTERFACE -DARMA_DONT_USE_SUPERLU)
    endif()
endif()

##################################################
# Create libraries

# add source list
# ls *.cxx >> CMakeLists.txt
set(source_list
        src/Extra.cxx
        src/Log.cxx
)


# add library ... ? makes a library
add_library(vibecommon SHARED ${source_list})
add_library(Vibe::Common ALIAS vibecommon)
# installed next to its deps in <prefix>/lib/vibe (smg in lib/vibe/smg)
set_target_properties(vibecommon PROPERTIES INSTALL_RPATH "$ORIGIN;$ORIGIN/smg")

# expose headers as vibe/common/X.hxx in the build tree (matches install layout)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/vibe)
if(NOT EXISTS ${CMAKE_BINARY_DIR}/include/vibe/common)
    file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include/vibe/common RESULT _vibe_link SYMBOLIC)
    if(NOT _vibe_link EQUAL 0)
        # no symlink permission (e.g. Windows without developer mode): copy; refreshed only on a clean configure
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_BINARY_DIR}/include/vibe/common)
    endif()
endif()

# include directories 
# as interfaces to add includes to anything that adds these libraries
target_include_directories(vibecommon 
    PUBLIC 
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        )

set_target_properties(vibecommon PROPERTIES
    LINKER_LANGUAGE CXX
)

# enable warnings
message(STATUS "compiler id: " ${CXX_COMPILER_ID})
target_compile_options(vibecommon PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wpedantic -Wall -Wextra>
        #-W>
        )

# link to library
target_link_libraries(vibecommon PUBLIC
    Arma::Armadillo
    #"Wl --enable-new-dtags"  # needed for rpath on linux
 )

# armadillo's BLAS wrapper must resolve when vibecommon links as a DLL (vcpkg openblas/lapack)
if(WIN32)
    find_package(BLAS REQUIRED)
    find_package(LAPACK REQUIRED)
    target_link_libraries(vibecommon PUBLIC BLAS::BLAS LAPACK::LAPACK)
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

message(STATUS "install vibe-common")

# Version header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hh.in 
    ${CMAKE_CURRENT_BINARY_DIR}/include/version.hh
)

## INSTALLATION
# get install directories
include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/vibecommon)

install(TARGETS vibecommon
    EXPORT vibecommon-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/vibe
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/vibe
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# This is required so that the exported target has the name vibe and not vibe
set_target_properties(vibecommon PROPERTIES EXPORT_NAME Common)

# install includes
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vibe/common)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/version.hh DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vibe/common)

# Export the targets to a script
    install(EXPORT vibecommon-targets
        FILE VibeCommonTargets.cmake
        NAMESPACE Vibe::
        DESTINATION ${INSTALL_CONFIGDIR}
    )

# Create a ConfigVersion.cmake file
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/VibeCommonConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/VibeCommonConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/VibeCommonConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
    )

# Install the config, configversion and custom find modules
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/VibeCommonConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/VibeCommonConfigVersion.cmake
        DESTINATION ${INSTALL_CONFIGDIR}
    )

## EXPORT
# Exporting from the build tree

    export(EXPORT vibecommon-targets
        FILE ${CMAKE_CURRENT_BINARY_DIR}/VibeCommonTargets.cmake
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
