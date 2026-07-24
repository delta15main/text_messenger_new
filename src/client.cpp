#include <cpr/parameters.h>
#include <netinet/in.h>
#include <future>
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <sstream>

#include "main.h"

namespace cli {

    // Вспомогательная функция для вывода погоды
    void print_weather(const nlohmann::json& j) {
        using namespace nlohmann;
        
        std::cout << "\n=== Погода ===" << std::endl;
        
        if (j.contains("name")) {
            std::cout << "🌆 Город: " << j["name"].get<std::string>() << std::endl;
        }
        
        if (j.contains("main")) {
            auto& main = j["main"];
            if (main.contains("temp")) {
                std::cout << "🌡️ Температура: " << main["temp"].get<double>() << "°C" << std::endl;
            }
            if (main.contains("feels_like")) {
                std::cout << "🌡️ Ощущается как: " << main["feels_like"].get<double>() << "°C" << std::endl;
            }
            if (main.contains("humidity")) {
                std::cout << "💧 Влажность: " << main["humidity"].get<int>() << "%" << std::endl;
            }
            if (main.contains("pressure")) {
                std::cout << "📊 Давление: " << main["pressure"].get<int>() << " гПа" << std::endl;
            }
        }
        
        if (j.contains("weather") && j["weather"].is_array() && !j["weather"].empty()) {
            auto& weather = j["weather"][0];
            if (weather.contains("main")) {
                std::cout << "☁️ Погода: " << weather["main"].get<std::string>() << std::endl;
            }
            if (weather.contains("description")) {
                std::cout << "📝 Описание: " << weather["description"].get<std::string>() << std::endl;
            }
        }
        
        if (j.contains("wind")) {
            auto& wind = j["wind"];
            if (wind.contains("speed")) {
                std::cout << "💨 Ветер: " << wind["speed"].get<double>() << " м/с" << std::endl;
            }
        }
        
        std::cout << "==================" << std::endl;
    }

    Client::Client(const std::string& ip, int port) {
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);
        
        if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            std::cerr << "❌ Ошибка подключения к серверу" << std::endl;
            is_connected = false;
            return;
        }
        is_connected = true;
        std::cout << "✅ Подключено к серверу" << std::endl;
    }

    Client::~Client() {
        if (client_socket != -1) {
            close(client_socket);
            client_socket = -1;
        }
    }

    void Client::send_city(const std::string& city) {
        if (!is_connected) return;
        send(client_socket, city.c_str(), city.size(), 0);
    }

    void Client::receive_weather() {
    char arr[4096] = {0};
    int bytes = recv(client_socket, arr, sizeof(arr) - 1, MSG_DONTWAIT);

    if (bytes == -1) { 
        return;
    }
    if (bytes == 0) {
        std::cout << "⚠️ Сервер закрыл соединение" << std::endl;
        is_connected = false;
        return;
    }
    
    arr[bytes] = '\0';
    std::string message(arr);
    
    try {
        nlohmann::json j = nlohmann::json::parse(message);  // Полное имя
        
        if (j.is_array()) {
            for (auto& city_weather : j) {
                print_weather(city_weather);
            }
            std::cout << "> ";
            std::cout.flush();
        }
    } catch (const nlohmann::json::parse_error& e) {  // Полное имя
        // Игнорируем ошибки парсинга
    }
}

    void Client::run() {
        if (!is_connected) {
            std::cerr << "❌ Нет подключения к серверу" << std::endl;
            return;
        }

        // Запускаем поток для приёма погоды
        std::future<void> receive_future = std::async(std::launch::async, [this]() {
            while (is_connected) {
                receive_weather();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });

        // Основной цикл для ввода команд
        std::cout << "📝 Введите команду (weather 'city' или exit):" << std::endl;
        
        while (is_connected) {
            std::string input;
            std::cout << "> ";
            std::getline(std::cin, input);
            
            if (input.empty()) continue;
            
            std::string command, city;
            std::istringstream iss(input);
            iss >> command >> city;
            
            if (command == "weather") {
                if (city.empty()) {
                    std::cout << "❌ Укажите город. Пример: weather Moscow" << std::endl;
                    continue;
                }
                send_city(city);
                std::cout << "📤 Город отправлен: " << city << std::endl;
            } else if (command == "exit") {
                std::cout << "👋 Отключение..." << std::endl;
                is_connected = false;
                break;
            } else {
                std::cout << "❌ Неизвестная команда. Используйте: weather 'city' или exit" << std::endl;
            }
        }

        if (receive_future.valid()) {
            receive_future.wait();
        }
        
        close(client_socket);
        std::cout << "✅ Клиент завершил работу" << std::endl;
    }

} // namespace cli
