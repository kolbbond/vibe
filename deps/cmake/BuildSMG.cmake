# Build simple-magnum-gui (smg) using ExternalProject
# Uses local submodule source (includes Corrade/Magnum submodules)
message(STATUS "Configuring simple-magnum-gui from submodule")

# SceneGraph disabled on Windows - causes GUI freeze issues
if(WIN32)
    option(ENABLE_SCENEGRAPH "Build smg with Magnum SceneGraph support" OFF)
else()
    option(ENABLE_SCENEGRAPH "Build smg with Magnum SceneGraph support" ON)
endif()

set(smg_CMAKE_ARGS
    -D ENABLE_TESTING=OFF
    -D ENABLE_EXAMPLES=OFF
    -D ENABLE_SCENEGRAPH=${ENABLE_SCENEGRAPH}
    -D BUILD_SHARED_LIBS=ON
    -D CMAKE_INSTALL_RPATH=${DEPS_PREFIX}/lib
    -D CMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE
)

# On Windows: pass vcpkg toolchain to ensure consistent SDL2 linking
# Without this, SMG may link SDL2d.dll (debug) while main app links SDL2.dll (release)
if(WIN32 AND DEFINED CMAKE_TOOLCHAIN_FILE)
    list(APPEND smg_CMAKE_ARGS
        -D CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    )
endif()

ExternalProject_Add(smg
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/simple-magnum-gui
    CMAKE_ARGS ${DEPS_CMAKE_ARGS} ${smg_CMAKE_ARGS}
    DOWNLOAD_COMMAND ""
    UPDATE_COMMAND ""
)
