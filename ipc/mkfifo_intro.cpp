#include <utils/error.hpp>

#include <fcntl.h>  
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <format>

int main() {
    static constexpr auto FifoPath = "/tmp/mkfifo_intro";

    CHECK_ERROR(mkfifo(FifoPath, 0666));

    const pid_t pid = fork();
    CHECK_ERROR(pid);

    if (pid == 0) {
        const int fd = open(FifoPath, O_RDONLY);
        CHECK_ERROR(fd);

        while (true) {
            char buf[1024]{};
            const auto bytes = read(fd, buf, sizeof(buf));
            CHECK_ERROR(bytes);

            if (bytes == 0) {
                break;
            }

            std::cout << std::format("[CHILD]: read {} bytes: {}", bytes, buf) << std::endl;
        }

        CHECK_ERROR(close(fd));
    } else {
        const int fd = open(FifoPath, O_WRONLY);
        CHECK_ERROR(fd);

        std::string input;
        while (std::getline(std::cin, input)) {
            const auto bytes = write(fd, input.c_str(), input.size());
            CHECK_ERROR(bytes);
            std::cout << std::format("[PARENT]: wrote {} bytes", bytes) << std::endl;
        }

        CHECK_ERROR(close(fd));
        CHECK_ERROR(wait(nullptr));
    }

    std::cout << std::format("[{}]: exited", pid == 0 ? "CHILD" : "PARENT") << std::endl;

    return 0;
}