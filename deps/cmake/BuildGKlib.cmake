# build gklib
# gklib options

set(gklib_CMAKE_ARGS
)

get_externalproject_options(gklib ${DEPS_IGNORE_SHA})
ExternalProject_Add(gklib
    DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/gklib
    SOURCE_DIR ${DEPS_BUILD_DIR}/src/gklib

    CONFIGURE_COMMAND make -C <SOURCE_DIR> config cc=gcc prefix=${DEPS_INSTALL_DIR}
    BUILD_COMMAND make -C <SOURCE_DIR>
    INSTALL_COMMAND make install -C <SOURCE_DIR>
    #LIST_SEPARATOR | # alternate list separator
    CMAKE_ARGS ${DEPS_CMAKE_ARGS} ${gklib_CMAKE_ARGS}
    ${EXTERNALPROJECT_OPTIONS}

)


