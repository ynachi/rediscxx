cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# Suppose this is your existing project
project(my_project)

# Set some options internally used in Photon
set(PHOTON_ENABLE_URING ON CACHE INTERNAL "Enable iouring")
set(PHOTON_ENABLE_SASL ON CACHE INTERNAL "Enable sasl")
set(PHOTON_CXX_STANDARD 17 CACHE INTERNAL "C++ standard")

# 1. Fetch Photon repo with specific tag or branch
include(FetchContent)
FetchContent_Declare(
        photon
        GIT_REPOSITORY https://github.com/alibaba/PhotonLibOS.git
        GIT_TAG main
)
FetchContent_MakeAvailable(photon)

# 2. Submodule
#add_subdirectory(photon)