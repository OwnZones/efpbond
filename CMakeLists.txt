cmake_minimum_required(VERSION 3.10)
project(efpbond)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

include(ExternalProject)
ExternalProject_Add(project_efp
        GIT_REPOSITORY https://bitbucket.org/unitxtra/efp.git
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/efp
        BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/efp
        GIT_PROGRESS 1
        BUILD_COMMAND make efp
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        INSTALL_COMMAND ""
        )
add_library(efp STATIC IMPORTED)
set_property(TARGET efp PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/efp/libefp.a)


add_library(efpbond STATIC EFPBonding.cpp)
add_executable(efpbondtest main.cpp EFPBonding.cpp EFPBonding.h)
add_dependencies(efpbondtest project_efp)
target_link_libraries(efpbondtest efp)