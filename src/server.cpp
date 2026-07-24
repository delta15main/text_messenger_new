#include <chrono>
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

#include "main.h"

namespace svr {
    using namespace usr;

    Server::Server(int port) : a(1) {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            std::cout << "ERROR!" << std::endl;
        } 
        if (listen(server_socket, 3) == -1) {
            close(server_socket);
            std::cout << "ERROR!" << std::endl;
        }
        get_mess = std::async(std::launch::async, [this]() { this->get_messange(); });
        weather_future = std::async(std::launch::async, [this]() { this->weather(); });
        
        std::cout << "🚀 Server start" << std::endl;
    }

    Server::~Server() {
        if (server_socket != -1) {
            close(server_socket);
            server_socket = -1;
        }

        for (auto i : users) {
            if (i) { i.reset(); }
        }
        if (!futures.empty()) {
            for (auto& o : futures) {
                if (o.valid()) { o.wait(); }
            }
        }    
        if (get_mess.valid()) { get_mess.wait(); }
        if (weather_future.valid()) { weather_future.wait(); } 
    }

    void Server::get_messange() {
        while (true) {
            for (auto& user : users) {
                char buffer[512] = {0};
                int bytes = recv(user->client_socket, buffer, sizeof(buffer)-1, MSG_DONTWAIT);
                if (bytes > 0) {
                    Stop s(cit_mutex);
                    std::string city(buffer);
                    city.erase(city.find_last_not_of(" \n\r\t") + 1);
                    if (!city.empty()) {
                        if (std::find(cit.begin(), cit.end(), city) == cit.end()) {
                            cit.push_back(city);
                            std::cout << "✅ Новый город: " << city << std::endl;
                        } else {
                            std::cout << "⚠️ Город уже есть: " << city << std::endl;
                        }
                        user->set_aut(true);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void Server::Run() {
        while(true){
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if ( client_socket == -1) {
                std::cout << "ERROR!" << std::endl;
                if (server_socket == -1) { break; }
                continue;
            }
            ++active_user;

            auto user = std::make_shared<User>(client_addr, client_socket);
            
            users.push_back(user);
            std::cout << "👤 Новый пользователь подключён. Всего: " << users.size() << std::endl;
        }
    }

    void Server::weather() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(a));
            a++;
            if (cit.empty()) continue;
            
            std::string weath = this->make_weath();
            const char* data = weath.data();
            size_t remaining = weath.size();
            
            for (auto& user : users) {
                if (!user->Is_user_aut()) continue;
                
                size_t sent_total = 0;
                while (sent_total < remaining) {
                    ssize_t sent = send(user->client_socket, data + sent_total, remaining - sent_total, 0);
                    if (sent == -1) {
                        std::cerr << "❌ Ошибка отправки" << std::endl;
                        break;
                    }
                    sent_total += sent;
                }
            }
        }
    }

    std::string Server::make_weath() {
        using json = nlohmann::json;
        json result_array = json::array();  // Массив вместо строки

        Stop s(cit_mutex);
        for (auto c : cit) {
            cpr::Header head = {
                {"Accept", "application/json"}
            };
            cpr::Parameters params = {
                {"q", c},
                {"appid", "b933f4cc03c1045e181c06e57d2650a9"},
                {"units", "metric"}
            };
            cpr::Response get_json = cpr::Get(cpr::Url{"https://api.openweathermap.org/data/2.5/weather"}, head, params);
            if (get_json.status_code != 200) {
                std::cerr << "❌ Ошибка API для города " << c << ": " << get_json.status_code << std::endl;
                continue;
            }
            try {
                json tmp = json::parse(get_json.text);
                result_array.push_back(tmp);
            } catch (const std::exception& e) {
                std::cerr << "❌ Ошибка парсинга JSON для города " << c << ": " << e.what() << std::endl;
            }
        }
        return result_array.dump();  // Один валидный JSON-массив
    }
}
