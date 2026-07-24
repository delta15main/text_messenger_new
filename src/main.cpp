#include <iostream>
#include <string>
#include <future>
#include <cstring>

#include "main.h"

int main(int argc, char* argv[]) {
    // Проверяем аргументы командной строки
    bool is_server = false;
    bool is_client = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0) {
            is_server = true;
        } else if (strcmp(argv[i], "--client") == 0) {
            is_client = true;
        }
    }

    // Если ничего не указано — запускаем сервер по умолчанию
    if (!is_server && !is_client) {
        is_server = true;
    }

    if (is_server) {
        std::cout << "🚀 Запуск сервера..." << std::endl;
        svr::Server server(8080);
        server.Run();
    } else if (is_client) {
        std::cout << "💻 Запуск клиента..." << std::endl;
        cli::Client client("127.0.0.1", 8080);
        client.run();
    }

    return 0;
}
