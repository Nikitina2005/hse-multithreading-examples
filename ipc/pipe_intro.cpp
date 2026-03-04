#include <utils/error.hpp>

#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <format>

int main() {
    int pipeFd[2]{};

    CHECK_ERROR(pipe(pipeFd));

    const pid_t pid = fork();
    CHECK_ERROR(pid);

    if (pid == 0) {
        CHECK_ERROR(close(pipeFd[1]));

        while (true) {
            char buf[1024]{};
            const auto bytes = read(pipeFd[0], buf, sizeof(buf));
            CHECK_ERROR(bytes);

            if (bytes == 0) {
                break;
            }

            std::cout << std::format("[CHILD]: read {} bytes: {}", bytes, buf) << std::endl;
        }

        CHECK_ERROR(close(pipeFd[0]));
    } else {
        CHECK_ERROR(close(pipeFd[0]));

        std::string input;
        while (std::getline(std::cin, input)) {
            const auto bytes = write(pipeFd[1], input.c_str(), input.size());
            CHECK_ERROR(bytes);
            std::cout << std::format("[PARENT]: wrote {} bytes", bytes) << std::endl;
        }

        CHECK_ERROR(close(pipeFd[1]));
        CHECK_ERROR(wait(nullptr));
    }

    std::cout << std::format("[{}]: exited", pid == 0 ? "CHILD" : "PARENT") << std::endl;

    return 0;
}