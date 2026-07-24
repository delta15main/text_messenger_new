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

#include "main.h"

namespace usr {

    User::User (const struct sockaddr_in& cli_addr, int cli_soc) :
        client_addr(cli_addr),
        client_socket(cli_soc),
        is_active(true),
        is_aut(false) { 
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
            std::cout << "✅ Новый клиент: " + std::string(ip) + ":" + std::to_string(ntohs(client_addr.sin_port)) << std::endl;
    }

    bool User::Is_user_active() { return is_active; }
    bool User::Is_user_aut() { return is_aut; }
    void User::set_aut(bool b) { is_aut = b; }

}
