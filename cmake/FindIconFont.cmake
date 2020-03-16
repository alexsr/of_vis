cmake_minimum_required(VERSION 3.8)

add_library(iconfont::iconfont INTERFACE IMPORTED)

target_include_directories(iconfont::iconfont INTERFACE ${EXTERNAL_PATH}/iconfont/)

target_compile_features(iconfont::iconfont INTERFACE cxx_std_17)
