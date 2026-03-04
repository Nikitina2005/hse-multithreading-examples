#include <utils/error.hpp>

#include <fcntl.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <format>
#include <thread>

int main() {
    static constexpr auto SemaPath = "/my_own_sema";

    sem_t *sema = sem_open(SemaPath, O_CREAT, 0666, 0);

    if (sema == SEM_FAILED) {
        throw std::runtime_error{std::format("Opening semaphore failed, error: {}", strerror(errno))};
    }

    const pid_t pid = fork();
    CHECK_ERROR(pid);

    if (pid == 0) {
        std::cout << "[CHILD]: waiting for parent signal" << std::endl;
        CHECK_ERROR(sem_wait(sema));
        std::cout << "[CHILD]: got the signal" << std::endl;

        CHECK_ERROR(sem_close(sema));
    } else {
        std::this_thread::sleep_for(std::chrono::seconds{3});

        std::cout << "[PARENT]: signaling child" << std::endl;
        CHECK_ERROR(sem_post(sema));


        CHECK_ERROR(wait(nullptr));
        CHECK_ERROR(sem_unlink(SemaPath));
    }

    return 0;
}