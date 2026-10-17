# build suitesparse
# suitesparse options

# add prefix paths
list(APPEND _suitesparse_prefix_paths "${CMAKE_PREFIX_PATH}") # propagate prefix paths
list(JOIN _suitesparse_prefix_paths ";" _suitesparse_prefix_path_string)
string(REPLACE ";" "|" _suitesparse_prefix_path_fixed "${_suitesparse_prefix_path_string}")

# set projects to build
set(suitesparse_projects "suitesparse_config;camd;ccolamd;colamd;cholmod;ldl;klu;umfpack;paru;spqr")
list(JOIN suitesparse_projects ";" suitesparse_projects)
string(REPLACE ";" "|" suitesparse_projects_fixed "${suitesparse_projects}")

# find openblas
# @hey: maybe we just confirm we have openblas prior
find_package(OpenBLAS CONFIG REQUIRED)
message(WARNING "OpenBLAS found: ${OpenBLAS_VERSION} at ${OpenBLAS_INCLUDE_DIRS} and ${OpenBLAS_LIBRARIES}")

set(suitesparse_CMAKE_ARGS
    -D CMAKE_PREFIX_PATH=${_suitesparse_prefix_path_fixed}
    -D BLA_VENDOR=OpenBLAS
#-D SUITESPARSE_ENABLE_PROJECTS="suitesparse_config;mongoose;amd;btf;camd;ccolamd;colamd;cholmod;cxsparse;ldl;klu;umfpack;paru;rbio;spqr;spex;graphblas;lagraph"
    -D SUITESPARSE_ENABLE_PROJECTS=${suitesparse_projects_fixed}
    -D SUITESPARSE_USE_CUDA=OFF # @hey: enable later, some issue with cudalt
    -D CMAKE_POSITION_INDEPENDENT_CODE=ON
    -D BLAS_LIBRARIES=${OPENBLAS_LIBRARIES}
    -D LAPACK_LIBRARIES=${OPENBLAS_LIBRARIES}
)

get_externalproject_options(suitesparse ${DEPS_IGNORE_SHA})
ExternalProject_Add(suitesparse
    DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/suitesparse
    SOURCE_DIR ${DEPS_BUILD_DIR}/src/suitesparse

    LIST_SEPARATOR | # alternate list separator
    CMAKE_ARGS ${DEPS_CMAKE_ARGS} ${suitesparse_CMAKE_ARGS}
    ${EXTERNALPROJECT_OPTIONS}

)



