cmake_minimum_required(VERSION 3.25)

project(dfs)

add_executable(dfs dfs.cpp)

target_compile_features(dfs PRIVATE cxx_std_20)

target_compile_options(dfs PRIVATE -fcoroutines)