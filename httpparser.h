#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include <string>
#include <unordered_map>
#include <iostream>
#include <strings.h>

namespace HTTP {
    enum class Method {
        GET = 0, POST, PUT, DELETE, PATCH
    };

    typedef std::unordered_map<std::string, std::string> QueryParams;
    typedef std::unordered_map<std::string, std::string> Headers;

    struct ParseResult {
        HTTP::Method method;
        std::string path;
        HTTP::QueryParams queryParams;
        std::string version;
        HTTP::Headers headers;
        std::string body;
        int marker;

        bool hasHeader(const std::string &name) const {
            for (const auto &[key, val] : headers) {
                if (strcasecmp(key.c_str(), name.c_str()) == 0) {
                    return true;
                }
            }
            return false;
        }

        std::string getHeader(const std::string &name) const {
            for (const auto &[key, val] : headers) {
                if (strcasecmp(key.c_str(), name.c_str()) == 0) {
                    return val;
                }
            }
            return "";
        }

        void print() {
            std::string method_str = getMethodString(method);

            std::cout << "Method: " << method_str << '\n';
            std::cout << "Path: " << path << '\n';
            std::cout << "Query params: \n";
            for (const auto &[key, value] : queryParams) {
                std::cout << key << " = " << value << '\n';
            }

            std::cout << "HTTP Version: " << version << '\n';

            std::cout << "Headers: \n";
            for (const auto &[key, value] : headers) {
                std::cout << key << " = " << value << '\n';
            }

            std::cout << "Body: \n";
            std::cout << body << '\n';

            std::cout << "Marker: \n";
            std::cout << marker << '\n';
        }

        static std::string getMethodString(Method method) {
            std::string method_str;
            switch (method) {
            case HTTP::Method::GET:
                method_str = "GET";
                break;
            case HTTP::Method::POST:
                method_str = "POST";
                break;
            case HTTP::Method::PUT:
                method_str = "PUT";
                break;
            case HTTP::Method::DELETE:
                method_str = "DELETE";
                break;
            case HTTP::Method::PATCH:
                method_str = "PATCH";
                break;
            }
            return method_str;
        }
    };

    class Parser
    {
    public:
        Parser() = default;

        static bool isRequestComplete(const std::string &buffer, size_t &out_request_len);
        ParseResult parse(const std::string &request);
    };

    /*
    * Exceptions
    */

    class ParserException : public std::exception
    {
    public:
        ParserException(const std::string &msg) : what_message(msg) {}
        std::string what_message;
        const char* what() const noexcept override {
            return what_message.c_str();
        }
    };
}

#endif // HTTPPARSER_H
