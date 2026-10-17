# Copyright (c) 2015 Andrew Kelley
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# SOUNDIO_FOUND
# SOUNDIO_INCLUDE_DIR
# SOUNDIO_LIBRARY

get_filename_component(_soundio_repo_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
if(DEFINED ENV{DEPS_BUILD_DIR})
    set(_soundio_deps_root "$ENV{DEPS_BUILD_DIR}")
else()
    set(_soundio_deps_root "${_soundio_repo_root}/.deps")
endif()

find_path(SOUNDIO_INCLUDE_DIR NAMES soundio/soundio.h
    HINTS
        "${_soundio_deps_root}/usr/include"
        "${_soundio_repo_root}/external/libsoundio"
)

find_library(SOUNDIO_LIBRARY NAMES soundio libsoundio
    HINTS
        "${_soundio_deps_root}/usr/lib"
        "${_soundio_repo_root}/.deps/libsoundio-build"
        "${_soundio_deps_root}/lib"
        "${_soundio_deps_root}/bin"
)

include(FindPackageHandleStandardArgs)
#find_package_handle_standard_args(SOUNDIO DEFAULT_MSG SOUNDIO_LIBRARY SOUNDIO_INCLUDE_DIR)
find_package_handle_standard_args(SoundIo DEFAULT_MSG SOUNDIO_LIBRARY SOUNDIO_INCLUDE_DIR)

mark_as_advanced(SOUNDIO_INCLUDE_DIR SOUNDIO_LIBRARY)
