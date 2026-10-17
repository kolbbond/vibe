# build mumps
# mumps options

# -DCMAKE_INSTALL_PREFIX=~/.local -DBUILD_SHARED_LIBS=ON -DLAPACK_VENDOR=OpenBLAS 
# -DLAPACK_ROOT=~/.local/lib -DLAPACK_INCLUDE_DIR=~/.local/include/openblas;
set(mumps_CMAKE_ARGS
    -D LAPACK_VENDOR=OpenBLAS
    -D LAPACK_ROOT=/usr/lib/x86_64-linux-gnu/
    -D CMAKE_POSITION_INDEPENDENT_CODE=ON
    -D BUILD_SHARED_LIBS=ON

)

get_externalproject_options(mumps ${DEPS_IGNORE_SHA})
ExternalProject_Add(mumps
    DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/mumps
    SOURCE_DIR ${DEPS_BUILD_DIR}/src/mumps

    #LIST_SEPARATOR | # alternate list separator
    CMAKE_ARGS ${DEPS_CMAKE_ARGS} ${mumps_CMAKE_ARGS}
    ${EXTERNALPROJECT_OPTIONS}

)

add_dependencies(mumps metis)



