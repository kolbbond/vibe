# build libsoundio
# libsoundio options

set(libsoundio_CMAKE_ARGS
    -D BUILD_STATIC_LIBS=OFF
    -D BUILD_DYNAMIC_LIBS=ON
    -D BUILD_EXAMPLE_PROGRAMS=OFF
    -D BUILD_TESTS=OFF
)

# On Debian/Ubuntu, help CMake find PulseAudio (elsewhere it is found on its own)
if(EXISTS /usr/lib/x86_64-linux-gnu/libpulse.so)
    list(APPEND libsoundio_CMAKE_ARGS
        -D PULSEAUDIO_LIBRARY=/usr/lib/x86_64-linux-gnu/libpulse.so
    )
endif()

# cross platform patching
get_externalproject_options(libsoundio ${DEPS_IGNORE_SHA})
ExternalProject_Add(libsoundio
    DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/libsoundio
    SOURCE_DIR ${DEPS_BUILD_DIR}/src/libsoundio

    #LIST_SEPARATOR | # alternate list separator
    CMAKE_ARGS ${DEPS_CMAKE_ARGS} ${libsoundio_CMAKE_ARGS}
    ${EXTERNALPROJECT_OPTIONS}

)




