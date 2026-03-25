#include "mpsc_queue.hpp"

#include <unistd.h>

#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

int main() {
    constexpr size_t size = 4096;
    const std::string wanted_type = "string";
    consumer_node node(size);
    std::vector<std::byte> message;
    while (true) {
        if (node.receive(wanted_type, message)) {
            std::string s(reinterpret_cast<const char *>(message.data()), message.size());
            std::cout << "consumer: receive type = " << wanted_type << " message = \"" << s << "\"" << std::endl;
        }
    }
    return 0;
}