#include "server.h"
#include "httpparser.h"
#include "httpresponse.h"
#include <unordered_set>
#include <cctype>
#include <csignal>

const std::unordered_set<std::string> allowed_ops = {"add", "sub", "mul", "div"};

static bool is_number(const std::string &s) {
    if (s.empty()) return false;
    size_t start = 0;
    if (s[0] == '-' || s[0] == '+') {
        start = 1;
    }
    if (start >= s.size()) return false;
    for (size_t i = start; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            return false;
        }
    }
    return true;
}

static int doOperation(int a, int b, const std::string &op)
{
    if (op == "add") {
        return a + b;
    } else if (op == "sub") {
        return a - b;
    } else if (op == "mul") {
        return a * b;
    } else if (op == "div") {
        return a / b;
    } else {
        return 0;
    }
}

std::string handleParseResult(const HTTP::ParseResult &parse_result)
{
    if (parse_result.method != HTTP::Method::GET) {
        return HTTP::ResponseBuilder()
            .setVersion(parse_result.version)
            .setStatusCode(405)
            .setContentType("text/plain; charset=utf-8")
            .setBody("")
            .build();
    }

    if (!parse_result.hasHeader("Host")) {
        return HTTP::ResponseBuilder()
            .setVersion(parse_result.version)
            .setStatusCode(400)
            .setContentType("text/plain; charset=utf-8")
            .setBody("")
            .build();
    }

    std::string user_op = parse_result.path;
    if (user_op.starts_with('/')) {
        user_op = user_op.substr(1);
    }

    if (!allowed_ops.contains(user_op)) {
        return HTTP::ResponseBuilder()
            .setVersion(parse_result.version)
            .setStatusCode(404)
            .setContentType("text/plain; charset=utf-8")
            .setBody("")
            .build();
    }

    if (!parse_result.queryParams.contains("a") || !parse_result.queryParams.contains("b")) {
        return HTTP::ResponseBuilder()
            .setVersion(parse_result.version)
            .setStatusCode(400)
            .setContentType("text/plain; charset=utf-8")
            .setBody("")
            .build();
    }

    const std::string &a_str = parse_result.queryParams.at("a");
    const std::string &b_str = parse_result.queryParams.at("b");

    if (!is_number(a_str) || !is_number(b_str)) {
        return HTTP::ResponseBuilder()
            .setVersion(parse_result.version)
            .setStatusCode(400)
            .setContentType("text/plain; charset=utf-8")
            .setBody("")
            .build();
    }

    int a = 0;
    int b = 0;
    try {
        a = std::stoi(a_str);
        b = std::stoi(b_str);
    } catch (...) {
        return HTTP::ResponseBuilder()
            .setVersion(parse_result.version)
            .setStatusCode(400)
            .setContentType("text/plain; charset=utf-8")
            .setBody("")
            .build();
    }

    if (user_op == "div" && b == 0) {
        return HTTP::ResponseBuilder()
            .setVersion(parse_result.version)
            .setStatusCode(400)
            .setContentType("text/plain; charset=utf-8")
            .setBody("")
            .build();
    }

    int op_result = doOperation(a, b, user_op);

    return HTTP::ResponseBuilder()
        .setVersion(parse_result.version)
        .setStatusCode(200)
        .setContentType("text/plain; charset=utf-8")
        .setBody(std::to_string(op_result))
        .build();
}

int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);

    int port = SERVER_PORT;
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            port = SERVER_PORT;
        }
    }

    Server server(port);
    HTTP::Parser parser;

    while (true) {
        try {
            server.accept_conn();

            std::string request;
            while (server.read_request(request)) {
                std::string response;
                try {
                    HTTP::ParseResult parse_result = parser.parse(request);
                    response = handleParseResult(parse_result);
                } catch (const HTTP::ParserException &) {
                    response = HTTP::ResponseBuilder()
                        .setVersion("HTTP/1.1")
                        .setStatusCode(400)
                        .setContentType("text/plain; charset=utf-8")
                        .setBody("")
                        .build();
                }

                server.send_data(response);
            }

            server.close_client();
        } catch (const std::exception &e) {
            server.close_client();
        }
    }

    return 0;
}
