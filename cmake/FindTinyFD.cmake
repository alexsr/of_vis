cmake_minimum_required(VERSION 3.8)

add_library(tinyfd
    ${EXTERNAL_PATH}/tinyfd/tinyfd/tinyfiledialogs.c)

add_library(tinyfd::tinyfd ALIAS tinyfd)

target_include_directories(tinyfd INTERFACE ${EXTERNAL_PATH}/tinyfd/)

target_compile_features(tinyfd INTERFACE cxx_std_17)
