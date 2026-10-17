
# build openblas
# openblas options

#cmake -B build -DCMAKE_INSTALL_PREFIX=~/.local -DBUILD_SHARED_LIBS=ON -DCPP_THREAD_SAFETY_TEST=ON \
set(openblas_CMAKE_ARGS
    -D USE_OPENMP=0
    -D USE_LOCKING=1
    -D USE_THREAD=0
)

get_externalproject_options(openblas ${DEPS_IGNORE_SHA})
ExternalProject_Add(openblas
    DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/openblas
    SOURCE_DIR ${DEPS_BUILD_DIR}/src/openblas

    #LIST_SEPARATOR | # alternate list separator
    CMAKE_ARGS ${DEPS_CMAKE_ARGS} ${openblas_CMAKE_ARGS}
    ${EXTERNALPROJECT_OPTIONS}

)



