add_executable(spsc_stack spsc_stack.cpp)
target_link_libraries(spsc_stack PRIVATE Threads::Threads)

add_executable(spsc_stack_tsan spsc_stack.cpp)
target_link_libraries(spsc_stack_tsan PRIVATE Threads::Threads)
target_compile_options(spsc_stack_tsan PRIVATE -fsanitize=thread)
target_link_options(spsc_stack_tsan PRIVATE -fsanitize=thread)
