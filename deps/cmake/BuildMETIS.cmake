# build metis
# metis options

set(metis_CMAKE_ARGS
)

get_externalproject_options(metis ${DEPS_IGNORE_SHA})
ExternalProject_Add(metis
    DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/metis
    SOURCE_DIR ${DEPS_BUILD_DIR}/src/metis
    #STAMP_DIR ${DEPS_BUILD_DIR}/src/metis-stamp


    PATCH_COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/metis_makefile.patch"
                    ${DEPS_BUILD_DIR}/src/metis-stamp/patch/metis_makefile.patch
                COMMAND patch -p0 -i ${DEPS_BUILD_DIR}/src/metis-stamp/patch/metis_makefile.patch -f
                WORKING_DIRECTORY <SOURCE_DIR>

    CONFIGURE_COMMAND make -C <SOURCE_DIR> config shared=1 cc=gcc prefix=${DEPS_INSTALL_DIR} VERBOSE=1
    # @hey: add parallel to these make commands
    BUILD_COMMAND make -C <SOURCE_DIR> -j
    INSTALL_COMMAND make install -C <SOURCE_DIR>
    #LIST_SEPARATOR | # alternate list separator
    CMAKE_ARGS ${DEPS_CMAKE_ARGS} ${metis_CMAKE_ARGS}
    ${EXTERNALPROJECT_OPTIONS}

)

add_dependencies(metis gklib)



