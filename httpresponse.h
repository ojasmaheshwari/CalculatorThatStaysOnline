#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <string>
#include <unordered_map>

namespace HTTP {

class ResponseBuilder
{
public:
    ResponseBuilder();

    ResponseBuilder& setVersion(const std::string &versionA);

    ResponseBuilder& setStatusCode(const unsigned int statusCodeA);

    ResponseBuilder& setContentType(const std::string &contentTypeA);

    ResponseBuilder& setBody(const std::string &bodyA);

    std::string build() const;

private:
    std::string version;
    unsigned int status_code;
    std::string reason_phrase;
    std::string content_type;
    std::string content_length;
    std::string body;

    std::unordered_map<unsigned int, std::string> statusCode_to_phrase;
};

}

#endif // HTTPRESPONSE_H
