# vtk options
# from bash script
set(vtk_CMAKE_ARGS
  )

get_externalproject_options(vtk ${DEPS_IGNORE_SHA})
ExternalProject_Add(vtk
    #DEPENDS lua_compat53
  DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/vtk
  SOURCE_DIR ${DEPS_BUILD_DIR}/src/vtk
  CMAKE_ARGS ${DEPS_CMAKE_ARGS} ${vtk_CMAKE_ARGS}
  ${EXTERNALPROJECT_OPTIONS})


