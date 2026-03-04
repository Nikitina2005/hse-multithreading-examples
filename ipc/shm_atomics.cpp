#include <utils/error.hpp>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <format>
#include <iostream>

template <typename T, size_t Capacity> class BufferedLockFreeSPSCStack {
private:
  static_assert(std::atomic<T>::is_always_lock_free);

public:
  bool Push(T value) {
    while (true) {
      auto top = m_head.load();

      if (top == Capacity) {
        return false;
      }

      if (m_head.compare_exchange_weak(top, top + 1)) {
        m_buffer[top] = std::move(value);
        return true;
      }
    }
  }

  std::optional<T> Pop() {
    std::optional<T> result;

    while (true) {
      auto top = m_head.load();

      if (top == 0) {
        return result;
      }

      if (m_head.compare_exchange_weak(top, top - 1)) {
        result = std::move(m_buffer[top - 1]);
        return result;
      }
    }
  }

private:
  std::atomic<int> m_head{0};
  std::array<T, Capacity> m_buffer{};
};

int main() {
  static constexpr int NumberOfElements = 1'000'000;

  using Stack = BufferedLockFreeSPSCStack<int, 1024>;

  void *sharedMemoryPtr = mmap(nullptr, sizeof(Stack), PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANONYMOUS, 0, 0);
  if (sharedMemoryPtr == MAP_FAILED) {
    throw std::runtime_error{
        std::format("mmap failed, error: {}", strerror(errno))};
  }

  Stack *stack = new (sharedMemoryPtr) Stack{};

  const pid_t pid = fork();
  CHECK_ERROR(pid);

  if (pid == 0) {
    for (int i{}; i < NumberOfElements; ++i) {
      while (!stack->Push(i)) {
      }
    }

    stack->~Stack();
  } else {
    int consumed{};
    while (consumed < NumberOfElements) {
      auto value = stack->Pop();
      if (value) {
        //std::cout << std::format("Consumed: {}, total consumed elements: {}", *value, consumed) << std::endl;
        ++consumed;
      }
    }

    CHECK_ERROR(wait(nullptr));
    stack->~Stack();
    CHECK_ERROR(munmap(sharedMemoryPtr, sizeof(Stack)));
  }

  std::cout << std::format("[{}]: exited", pid == 0 ? "CHILD" : "PARENT")
            << std::endl;

  return 0;
}