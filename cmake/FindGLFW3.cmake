cmake_minimum_required(VERSION 3.8)

find_library(GLFW_LIBRARY
    NAMES glfw3 libglfw3.a libglfw.a glfw
    PATHS ${EXTERNAL_PATH}/glfw3/
    PATH_SUFFIXES lib)
if (GLFW_LIBRARY)
    add_library(glfw::glfw3 STATIC IMPORTED)
    set_target_properties(glfw::glfw3 PROPERTIES
        IMPORTED_LOCATION ${GLFW_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${EXTERNAL_PATH}/glfw3/include/)
endif()
