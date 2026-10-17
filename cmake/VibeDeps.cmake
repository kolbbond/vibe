# third-party search paths for the vibe engine and anything built against it
#
#   include(<vibe>/cmake/VibeDeps.cmake)   in a consumer's top level, before add_subdirectory(<vibe> vibe)
#
# Sets VIBE_ROOT (the vibe source tree) and DEPS_PREFIX (the deps/ superbuild, default <vibe>/.deps/usr),
# and points module/prefix paths and the build RPATH at it, in the including scope.
include_guard(GLOBAL)

get_filename_component(VIBE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# check for where we are in buildsystem
if(DEFINED ENV{DEPS_BUILD_DIR})
  set(DEPS_PREFIX "$ENV{DEPS_BUILD_DIR}/usr" CACHE PATH "Path prefix for finding dependencies")
else()
  set(DEPS_PREFIX "${VIBE_ROOT}/.deps/usr" CACHE PATH "Path prefix for finding dependencies")
endif()

# imported targets found in one engine subdir must be visible to siblings and consumers
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL ON)

# Map RelWithDebInfo/MinSizeRel to Release for imported targets that don't have them
# Include "None" for system packages built without a specific build type (like SDL2 on Arch)
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO RelWithDebInfo Release None Debug "")
set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL MinSizeRel Release None Debug "")

if(UNIX)
    # Use GLVND for modern OpenGL on Linux (needed for GLX)
    set(OpenGL_GL_PREFERENCE "GLVND")
    # RPATH: use local deps in the build tree, and $ORIGIN-based paths on install.
    set(CMAKE_BUILD_RPATH "${DEPS_PREFIX}/lib;${DEPS_PREFIX}/lib/smg")
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# set paths
# modules
list(APPEND CMAKE_MODULE_PATH "${VIBE_ROOT}/cmake")
list(INSERT CMAKE_MODULE_PATH 0 "${DEPS_PREFIX}/lib/cmake/;")
list(INSERT CMAKE_MODULE_PATH 0 "${DEPS_PREFIX}/cmake/")
list(INSERT CMAKE_MODULE_PATH 0 "${DEPS_PREFIX}/lib/cmake/")
list(INSERT CMAKE_MODULE_PATH 0 "${DEPS_PREFIX}/lib/cmake/smg")
list(INSERT CMAKE_MODULE_PATH 0 "${DEPS_PREFIX}/cmake/")
list(INSERT CMAKE_MODULE_PATH 0 "${DEPS_PREFIX}/share/cmake/")
list(INSERT CMAKE_MODULE_PATH 0 "${DEPS_PREFIX}/share/cmake/Corrade")
list(INSERT CMAKE_MODULE_PATH 0 "${DEPS_PREFIX}/share/cmake/Corrade/")
list(INSERT CMAKE_MODULE_PATH 0 "${DEPS_PREFIX}/share/cmake/MagnumIntegration")
list(INSERT CMAKE_MODULE_PATH 0 "${DEPS_PREFIX}/share/cmake/Magnum")

# prefix paths
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/share/pkgconfig")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/bin")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/lib")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/include")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/share/")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/cmake/")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/include/Corrade")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/lib/cmake")
list(INSERT CMAKE_PREFIX_PATH 0 "${DEPS_PREFIX}/lib/cmake/smg")
list(INSERT CMAKE_PREFIX_PATH 0 "/usr/lib/x86_64-linux-gnu/") # for opengl and sdl
