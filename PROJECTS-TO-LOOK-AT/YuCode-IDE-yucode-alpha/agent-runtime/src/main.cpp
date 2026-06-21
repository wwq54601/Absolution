#include "server.h"
#include <iostream>

int main() {
    std::cout << "YuCode Agent Runtime starting...\n";

    YuCodeServer server;
    server.start(8765);

    return 0;
}