#include "mpsc_queue.hpp"

#include <cstdio>
#include <iostream>
#include <cstring>
#include <array>
#include <string>

int main() {
    constexpr size_t size = 4096;
    const std::array<std::pair<std::string, std::string>, 6> messages{{
        {"string", "hello"},
        {"int", "17"},
        {"string", "botat'"},
        {"bool", "true"},
        {"string", "ot korobki do nk luchshe vseh frtk"},
        {"float", "3.1415927"},
    }};
    const size_t total_messages = messages.size();
    producer_node node(size);
    std::cout << "producer: sending " << total_messages << " messages" << std::endl;
    for (size_t i = 0; i < total_messages; i++) {
        sleep(rand() % 5);
        const std::string& type = messages[i].first;
        const std::string& message = messages[i].second;
        const size_t length = message.size();
        std::cout << "producer: send type = " << type << " message = \"" << message << "\"" << std::endl;
        bool success = node.send(type, message.data(), length);
        if (!success) {
            std::cerr << "producer: send failed at i = " << i << std::endl;
            return 404;
        }
    }
    return 0;
}