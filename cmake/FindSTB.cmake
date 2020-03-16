cmake_minimum_required(VERSION 3.8)

add_library(stb::stb INTERFACE IMPORTED)

target_include_directories(stb::stb INTERFACE ${EXTERNAL_PATH}/stb/)

target_compile_features(stb::stb INTERFACE cxx_std_17)
