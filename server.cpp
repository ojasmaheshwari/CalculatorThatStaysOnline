#include "server.h"
#include "httpparser.h"
#include <stdexcept>
#include <unistd.h>

Server::Server(const int port, const size_t backlog)
    : m_backlog(backlog), m_client_info_len(sizeof(m_client_info))
{
    m_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket_fd == -1) {
        throw std::runtime_error("[Server]: socket() failed");
    }

    int retcode;

    {
        const int option_value = 1;
        retcode = setsockopt(m_socket_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value));
        if (retcode == -1) {
            throw std::runtime_error("[Server]: setsockopt() SO_REUSEADDR failed");
        }
#ifdef SO_REUSEPORT
        setsockopt(m_socket_fd, SOL_SOCKET, SO_REUSEPORT, &option_value, sizeof(option_value));
#endif
    }

    m_server_info.sin_family = AF_INET;
    m_server_info.sin_addr.s_addr = INADDR_ANY;
    m_server_info.sin_port = htons(port);

    retcode = bind(m_socket_fd, (struct sockaddr *)&m_server_info, sizeof(m_server_info));
    if (retcode < 0) {
        throw std::runtime_error("[Server]: bind() failed");
    }

    retcode = listen(m_socket_fd, m_backlog);
    if (retcode == -1) {
        throw std::runtime_error("[Server]: listen() failed");
    }
}

Server::~Server()
{
    close_client();
    if (m_socket_fd != -1) {
        close(m_socket_fd);
        m_socket_fd = -1;
    }
}

void Server::close_client()
{
    if (m_client_fd != -1) {
        close(m_client_fd);
        m_client_fd = -1;
    }
    m_client_buffer.clear();
}

void Server::accept_conn()
{
    close_client();
    m_client_info_len = sizeof(m_client_info);
    m_client_fd = accept(m_socket_fd, &m_client_info, &m_client_info_len);
    if (m_client_fd < 0) {
        throw std::runtime_error("[Server]: accept() failed");
    }
    m_client_buffer.clear();
}

std::string Server::read()
{
    if (m_client_fd < 0) {
        throw SocketReadException("Client socket is not connected");
    }

    char buf[MAX_READ_LEN];

    int bytes_read = recv(m_client_fd, buf, MAX_READ_LEN, 0);
    if (bytes_read == -1) {
        throw SocketReadException("recv() failed");
    }

    return std::string(buf, bytes_read);
}

bool Server::read_request(std::string &out_request)
{
    while (true) {
        size_t req_len = 0;
        if (HTTP::Parser::isRequestComplete(m_client_buffer, req_len)) {
            out_request = m_client_buffer.substr(0, req_len);
            m_client_buffer.erase(0, req_len);
            return true;
        }

        if (m_client_fd < 0) {
            return false;
        }

        char buf[MAX_READ_LEN];
        ssize_t bytes_read = recv(m_client_fd, buf, MAX_READ_LEN, 0);
        if (bytes_read <= 0) {
            return false;
        }

        m_client_buffer.append(buf, bytes_read);
    }
}

void Server::send_data(const std::string &data) const
{
    if (m_client_fd < 0) {
        throw std::runtime_error("[Server]: send() called on invalid client socket");
    }

    size_t total_sent = 0;
    while (total_sent < data.size()) {
        ssize_t bytes_sent = send(m_client_fd, data.c_str() + total_sent, data.size() - total_sent, MSG_NOSIGNAL);
        if (bytes_sent <= 0) {
            throw std::runtime_error("[Server]: send() failed or client disconnected");
        }
        total_sent += bytes_sent;
    }
}