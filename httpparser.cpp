#include "httpparser.h"
#include <strings.h>

using namespace HTTP;

static void trim(std::string &s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        s.clear();
        return;
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    s = s.substr(start, end - start + 1);
}

bool Parser::isRequestComplete(const std::string &buffer, size_t &out_request_len)
{
    size_t header_end = buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return false;
    }

    size_t content_length = 0;
    std::string headers_part = buffer.substr(0, header_end + 4);
    size_t pos = 0;

    while (pos < headers_part.size()) {
        size_t next_line = headers_part.find("\r\n", pos);
        if (next_line == std::string::npos) break;
        std::string line = headers_part.substr(pos, next_line - pos);
        pos = next_line + 2;

        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            trim(key);
            if (strcasecmp(key.c_str(), "Content-Length") == 0) {
                std::string val = line.substr(colon + 1);
                trim(val);
                if (!val.empty() && val[0] != '-') {
                    try {
                        content_length = std::stoull(val);
                    } catch (...) {
                        content_length = 0;
                    }
                }
                break;
            }
        }
    }

    size_t total_len = header_end + 4 + content_length;
    if (buffer.size() < total_len) {
        return false;
    }

    out_request_len = total_len;
    return true;
}

ParseResult Parser::parse(const std::string &request)
{
    int marker = 0;

    size_t line_end = request.find("\r\n", marker);
    if (line_end == std::string::npos) {
        throw ParserException("Missing CRLF in request line");
    }

    // Method
    size_t sp1 = request.find(' ', marker);
    if (sp1 == std::string::npos || sp1 >= line_end) {
        throw ParserException("Failed to parse HTTP method");
    }

    std::string method_str = request.substr(marker, sp1 - marker);
    Method method;
    if (method_str == "GET") {
        method = Method::GET;
    } else if (method_str == "POST") {
        method = Method::POST;
    } else if (method_str == "PUT") {
        method = Method::PUT;
    } else if (method_str == "DELETE") {
        method = Method::DELETE;
    } else if (method_str == "PATCH") {
        method = Method::PATCH;
    } else {
        throw ParserException("Unknown HTTP method");
    }

    // URI
    size_t sp2 = request.find(' ', sp1 + 1);
    if (sp2 == std::string::npos || sp2 >= line_end) {
        throw ParserException("Failed to parse URI / missing HTTP version");
    }

    std::string uri = request.substr(sp1 + 1, sp2 - (sp1 + 1));
    std::string path;
    QueryParams queryParams;

    // Strip fragment if present
    size_t hash_pos = uri.find('#');
    if (hash_pos != std::string::npos) {
        uri = uri.substr(0, hash_pos);
    }

    size_t qpos = uri.find('?');
    if (qpos != std::string::npos) {
        path = uri.substr(0, qpos);
        std::string query_str = uri.substr(qpos + 1);

        size_t qp_start = 0;
        while (qp_start < query_str.size()) {
            size_t amp = query_str.find('&', qp_start);
            std::string pair = (amp == std::string::npos)
                ? query_str.substr(qp_start)
                : query_str.substr(qp_start, amp - qp_start);

            if (!pair.empty()) {
                size_t eq = pair.find('=');
                if (eq != std::string::npos) {
                    queryParams[pair.substr(0, eq)] = pair.substr(eq + 1);
                } else {
                    queryParams[pair] = "";
                }
            }

            if (amp == std::string::npos) break;
            qp_start = amp + 1;
        }
    } else {
        path = uri;
    }

    // HTTP Version
    std::string version = request.substr(sp2 + 1, line_end - (sp2 + 1));
    marker = line_end + 2;

    // Headers
    Headers headers;
    while (marker < request.size()) {
        if (marker + 1 < request.size() && request[marker] == '\r' && request[marker + 1] == '\n') {
            marker += 2;
            break;
        }

        size_t next_crlf = request.find("\r\n", marker);
        if (next_crlf == std::string::npos) {
            throw ParserException("Failed to parse headers: missing CRLF");
        }

        std::string header_line = request.substr(marker, next_crlf - marker);
        marker = next_crlf + 2;

        size_t colon = header_line.find(':');
        if (colon == std::string::npos) {
            throw ParserException("Failed to parse header line: missing colon");
        }

        std::string key = header_line.substr(0, colon);
        std::string val = header_line.substr(colon + 1);
        trim(key);
        trim(val);
        headers[key] = val;
        if (strcasecmp(key.c_str(), "Host") == 0) {
            headers["Host"] = val;
        }
        if (strcasecmp(key.c_str(), "Content-Length") == 0) {
            headers["Content-Length"] = val;
        }
    }

    // Body
    size_t content_length = 0;
    if (headers.contains("Content-Length")) {
        const std::string &cl_str = headers["Content-Length"];
        if (!cl_str.empty() && cl_str[0] == '-') {
            throw ParserException("Negative Content-Length");
        }
        try {
            content_length = std::stoull(cl_str);
        } catch (...) {
            throw ParserException("Invalid Content-Length");
        }
    }

    std::string body;
    if (content_length > 0) {
        if (marker + content_length > request.size()) {
            throw ParserException("Body size less than Content-Length");
        }
        body = request.substr(marker, content_length);
        marker += content_length;
    }

    return {
        .method = method,
        .path = path,
        .queryParams = queryParams,
        .version = version,
        .headers = headers,
        .body = body,
        .marker = marker
    };
}