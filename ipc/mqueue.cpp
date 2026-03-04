#include <utils/error.hpp>

#include <fcntl.h>
#include <mqueue.h>
#include <sys/wait.h>
#include <unistd.h>

#include <format>
#include <iostream>

struct Message {
  enum Type { Default, Stop };

  static constexpr size_t BufferSize = 1024;

  Type messageType{Default};
  char data[BufferSize]{};
};

int main() {
  static constexpr auto QueuePath = "/kafka";

  mq_unlink(QueuePath);

  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = sizeof(Message);
  attr.mq_curmsgs = 0;

  const pid_t pid = fork();
  CHECK_ERROR(pid);

  if (pid == 0) {
    mqd_t queue = mq_open(QueuePath, O_RDONLY | O_CREAT, 0666, &attr);
    CHECK_ERROR(queue);

    Message message;
    while (true) {
      ssize_t size = mq_receive(queue, reinterpret_cast<char *>(&message),
                                sizeof(message), nullptr);
      CHECK_ERROR(size);

      if (message.messageType == Message::Stop) {
        break;
      }

      std::cout << std::format("[CHILD]: received {} bytes: {}", size,
                               message.data)
                << std::endl;
    }

    CHECK_ERROR(mq_close(queue));
  } else {
    mqd_t queue = mq_open(QueuePath, O_WRONLY | O_CREAT, 0666, &attr);
    CHECK_ERROR(queue);

    Message message;
    std::string input;
    while (std::getline(std::cin, input)) {
      std::memset(message.data, 0, Message::BufferSize);
      std::memcpy(message.data, input.data(),
                  std::min(Message::BufferSize, input.size()));

      CHECK_ERROR(mq_send(queue, reinterpret_cast<char *>(&message),
                          sizeof(Message), 0));
    }

    std::memset(message.data, 0, Message::BufferSize);
    message.messageType = Message::Stop;
    CHECK_ERROR(
        mq_send(queue, reinterpret_cast<char *>(&message), sizeof(Message), 0));

    CHECK_ERROR(mq_close(queue));
    CHECK_ERROR(mq_unlink(QueuePath));
    CHECK_ERROR(wait(nullptr));
  }

  std::cout << std::format("[{}]: exited", pid == 0 ? "CHILD" : "PARENT")
            << std::endl;

  return 0;
}
