cmake_minimum_required(VERSION 3.25)
project(mpsc_shm_queue_homework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(producer producer.cpp)
add_executable(consumer consumer.cpp)

target_compile_options(producer PRIVATE -O2 -Wall -Wextra -pedantic)
target_compile_options(consumer PRIVATE -O2 -Wall -Wextra -pedantic)