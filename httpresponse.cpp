#include "httpresponse.h"
#include <assert.h>

using namespace HTTP;

ResponseBuilder::ResponseBuilder()
    : version("HTTP/1.1"),
      status_code(200),
      reason_phrase("OK"),
      content_type("text/plain; charset=utf-8"),
      content_length("0"),
      body("")
{
    statusCode_to_phrase[200] = "OK";
    statusCode_to_phrase[400] = "Bad Request";
    statusCode_to_phrase[404] = "Not Found";
    statusCode_to_phrase[405] = "Method Not Allowed";
}

ResponseBuilder& ResponseBuilder::setVersion(const std::string &versionA) {
    if (!versionA.empty()) {
        version = versionA;
    }
    return *this;
}

ResponseBuilder& ResponseBuilder::setStatusCode(const unsigned int statusCodeA) {
    status_code = statusCodeA;
    auto it = statusCode_to_phrase.find(status_code);
    if (it != statusCode_to_phrase.end()) {
        reason_phrase = it->second;
    } else {
        reason_phrase = "Unknown";
    }
    return *this;
}

ResponseBuilder& ResponseBuilder::setContentType(const std::string &contentTypeA) {
    content_type = contentTypeA;
    return *this;
}

ResponseBuilder& ResponseBuilder::setBody(const std::string &bodyA) {
    body = bodyA;
    content_length = std::to_string(body.size());
    return *this;
}

std::string ResponseBuilder::build() const
{
    std::string response;

    response += (version + " " + std::to_string(status_code) + " " + reason_phrase + "\r\n");
    response += ("Content-Type: " + content_type + "\r\n");
    response += ("Content-Length: " + std::to_string(body.size()) + "\r\n");
    response += "\r\n";
    response += body;

    return response;
}
