# build sundials
# sundials options

set(sundials_CMAKE_ARGS
)

get_externalproject_options(sundials ${DEPS_IGNORE_SHA})
ExternalProject_Add(sundials
    DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/sundials
    SOURCE_DIR ${DEPS_BUILD_DIR}/src/sundials

    #LIST_SEPARATOR | # alternate list separator
    CMAKE_ARGS ${DEPS_CMAKE_ARGS} ${sundials_CMAKE_ARGS}
    ${EXTERNALPROJECT_OPTIONS}

)




