cmake_minimum_required(VERSION 3.16)
project(process_pool_homework CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
include(FetchContent)
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.17.0)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(process_pool_test process_pool_test.cpp)
target_link_libraries(process_pool_test PRIVATE GTest::gtest_main Threads::Threads)
add_test(NAME process_pool COMMAND process_pool_test)
