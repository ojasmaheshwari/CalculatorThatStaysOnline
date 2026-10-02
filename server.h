#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>

#define MAX_READ_LEN 1024
#define SERVER_PORT 8080

class Server
{
public:
    Server(const int port = SERVER_PORT, const size_t backlog = 128);
    ~Server();
    void accept_conn();
    void close_client();
    std::string read();
    bool read_request(std::string &out_request);
    void send_data(const std::string &data) const;

private:
    int m_socket_fd{-1};
    struct sockaddr_in m_server_info;
    size_t m_backlog{};

    // Current client info
    struct sockaddr m_client_info;
    socklen_t m_client_info_len;
    int m_client_fd{-1};
    std::string m_client_buffer;
};

/*
 * Exceptions
*/
class SocketReadException : public std::exception
{
public:
    SocketReadException(const std::string &msg) : what_message(msg) {}
    std::string what_message;
    const char* what() const noexcept override {
        return what_message.c_str();
    }
};

#endif // SERVER_H
