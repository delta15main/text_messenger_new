#include <netinet/in.h>
#include <future>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

// RAII-обёртка для мьютекса
class Stop {
    std::mutex& mtx;
public:
    Stop(std::mutex& m) : mtx(m) { mtx.lock(); }
    ~Stop() { mtx.unlock(); }
};

// Класс клиента
namespace cli {
    class Client {
    private:
        int client_socket;
        struct sockaddr_in server_addr;
        bool is_connected;
    public:
        Client(const std::string& ip, int port);
        ~Client();
        
        void send_city(const std::string& city);
        void receive_weather();
        void run();
    };
}

// Класс пользователя (серверная часть)
namespace usr {
    class User {
    private:
        struct sockaddr_in client_addr;
        bool is_active;
        bool is_aut;
    public:
        int client_socket;
        User(const struct sockaddr_in& cli_addr, int cli_soc);
        bool Is_user_active();
        bool Is_user_aut();
        void set_aut(bool b);
    };
}

// Класс сервера
namespace svr {
    using namespace usr;
    class Server {
    private:
        int server_socket;
        struct sockaddr_in server_addr;
        int active_user;
        std::mutex cit_mutex;
        
        std::vector<std::shared_ptr<User>> users;
        std::vector<std::future<void>> futures;
        std::vector<std::string> cit;
        std::future<void> get_mess;
        std::future<void> weather_future;
    public:
        Server(int port);
        ~Server();
        void Run();
        void get_messange();
        void weather();
        std::string make_weath();
    };
}
