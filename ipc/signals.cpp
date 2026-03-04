#include <utils/error.hpp>

#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <format>
#include <iostream>
#include <thread>

std::atomic<bool> g_sigUsrReceived{};

void SigUsrHandler(int) { g_sigUsrReceived.store(true); }

int main() {
  const pid_t pid = fork();
  CHECK_ERROR(pid);

  if (pid == 0) {
    if (signal(SIGUSR1, SigUsrHandler) == SIG_ERR) {
      throw std::runtime_error{"signal() failed"};
    }

    while (!g_sigUsrReceived.load()) {
        std::cout << "[CHILD]: waiting for SIGUSR" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds{1});
    }
    // sigset_t expectedSignals{};
    // int receivedSignal{};
    // sigaddset(&expectedSignals, SIGUSR1);
    // sigwait(&expectedSignals, &receivedSignal);

    // if (receivedSignal != SIGUSR1) {
    //   throw std::runtime_error{
    //       std::format("unexpected signal: {}", receivedSignal)};
    // }

    while (true) {
      std::cout << "[CHILD]: waiting for SIGTERM" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds{1});
    }
  } else {
    std::this_thread::sleep_for(std::chrono::seconds{3});

    CHECK_ERROR(kill(pid, SIGUSR1));
    std::cout << "[PARENT]: sent SIGUSR1 to child" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds{3});
    CHECK_ERROR(kill(pid, SIGTERM));
    std::cout << "[PARENT]: sent SIGTERM to child" << std::endl;
    wait(nullptr);
  }

  std::cout << std::format("[{}]: exited", pid == 0 ? "CHILD" : "PARENT")
            << std::endl;
  return 0;
}
