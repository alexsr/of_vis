cmake_minimum_required(VERSION 3.8)

set(temp1734_version 0.1.0)

project(temp1734 VERSION ${temp1734_version} LANGUAGES CXX)

option(temp1734_BUILD_TESTS "Build tests" OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(EXTERNAL_PATH ${CMAKE_CURRENT_LIST_DIR}/external)
set(SRC_PATH ${CMAKE_CURRENT_LIST_DIR}/src)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)

find_package(opengl REQUIRED)
find_package(imgui REQUIRED)
find_package(glad REQUIRED)
include(${EXTERNAL_PATH}/glm/glmTargets.cmake)
include(${EXTERNAL_PATH}/eigen3/share/eigen3/cmake/Eigen3Targets.cmake)
option(temp1734_ASSIMP_RELEASE "Use assimp release lib" ON)
include(${CMAKE_MODULE_PATH}/assimpTargets.cmake)
find_package(openmp REQUIRED)
find_package(stb REQUIRED)
find_package(tinyfd REQUIRED)
find_package(iconfont REQUIRED)
find_package(glfw3 REQUIRED)

add_subdirectory(src/)

#if (temp1734_BUILD_TESTS)
#    enable_testing()
#    add_subdirectory(tests)
#endif()

add_subdirectory(projects/)
