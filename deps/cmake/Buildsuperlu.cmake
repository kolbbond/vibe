# superlu options
# from bash script
#  cmake -B build -DCMAKE_INSTALL_PREFIX=~/.local -DBUILD_SHARED_LIBS=ON -Denable_internal_blaslib=NO -DTPL_BLAS_LIBRARIES=libopenblas.so

# disable testing if on windows and superlu version = 5.3
if(WIN32)
    set(superlu_testing OFF)
else()
    set(superlu_testing ON)
endif()
set(superlu_CMAKE_ARGS
    #-D BUILD_SHARED_LIBS=ON
  -D CMAKE_POLICY_VERSION_MINIMUM=3.6
  -D enable_internal_blaslib=NO
  -D XSDK_ENABLE_Fortran=OFF
  -D enable_testing=NO # if building on WIN32
  -D enable_examples=NO
)

# pull repo
get_externalproject_options(superlu ${DEPS_IGNORE_SHA})
ExternalProject_Add(superlu
    #DEPENDS lua_compat53
  DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/superlu
  SOURCE_DIR ${DEPS_BUILD_DIR}/src/superlu

  CMAKE_ARGS ${DEPS_CMAKE_ARGS} ${superlu_CMAKE_ARGS}
  ${EXTERNALPROJECT_OPTIONS}
  )

if(USE_BUNDLED_OPENBLAS)
    add_dependencies(superlu openblas)
endif()
