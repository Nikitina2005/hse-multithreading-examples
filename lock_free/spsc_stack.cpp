#include <array>
#include <atomic>
#include <latch>
#include <mutex>
#include <optional>
#include <stack>
#include <thread>

template <typename T> class SPSCStack {
public:
  void Push(T value) {
    std::lock_guard lock{m_mtx};
    m_stack.push(std::move(value));
  }

  std::optional<T> Pop() {
    std::optional<T> result;

    std::lock_guard lock{m_mtx};

    if (m_stack.empty()) {
      return result;
    }

    result = std::move(m_stack.top());
    m_stack.pop();
    return result;
  }

private:
  std::mutex m_mtx;
  std::stack<T> m_stack;
};

// Can it be used as MPSC/SPMC/MPMC?
template <typename T> class LockFreeSPSCStack {
private:
  struct Node {
    T data;
    Node *next{};
    Node(T value) : data(std::move(value)) {}
  };

public:
  void Push(T value) {
    Node *newHead = new Node(std::move(value));

    newHead->next = m_head.load();
    while (!m_head.compare_exchange_weak(newHead->next, newHead)) {
    }
  }

  std::optional<T> Pop() {
    std::optional<T> result;

    Node *oldHead = m_head.load();
    while (oldHead && !m_head.compare_exchange_weak(oldHead, oldHead->next)) {
    }

    if (!oldHead) {
      return result;
    }

    result = std::move(oldHead->data);
    delete oldHead;
    return result;
  }

  ~LockFreeSPSCStack() {
    Node* head = m_head.exchange(nullptr);
    while (head) {
      Node *next = head->next;
      delete head;
      head = next;
    }
  }

private:
  std::atomic<Node *> m_head{nullptr};
};

template <typename T, size_t Capacity> class BufferedLockFreeSPSCStack {
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
  static constexpr int NumberOfElements = 5'000'000;

  SPSCStack<int> stack;
  //BufferedLockFreeSPSCStack<int, 1024> stack;
  //LockFreeSPSCStack<int> stack;
  std::latch latch{2};

  std::jthread producer([&stack, &latch] {
    latch.arrive_and_wait();

    for (int i{}; i < NumberOfElements; ++i) {
      //while (!stack.Push(i)) {}
      stack.Push(i);
    }
  });

  std::jthread consumer([&stack, &latch] {
    latch.arrive_and_wait();

    int consumed{};
    while (consumed < NumberOfElements) {
      auto value = stack.Pop();
      if (value) {
        ++consumed;
      }
    }
  });

  return 0;
}
