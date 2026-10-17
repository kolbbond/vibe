# nlopt options
# from bash script
set(nlopt_CMAKE_ARGS
  )

get_externalproject_options(nlopt ${DEPS_IGNORE_SHA})
ExternalProject_Add(nlopt
    #DEPENDS lua_compat53
  DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/nlopt
  SOURCE_DIR ${DEPS_BUILD_DIR}/src/nlopt
  CMAKE_ARGS ${DEPS_CMAKE_ARGS} ${nlopt_CMAKE_ARGS}
  ${EXTERNALPROJECT_OPTIONS})


