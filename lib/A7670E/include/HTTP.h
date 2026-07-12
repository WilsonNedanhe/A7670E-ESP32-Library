#pragma once
#include "ATClient.h"

// Phase 8 — HTTP / HTTPS client

struct HTTPResponse {
    int  statusCode;
    int  bodyLen;
    char body[HTTP_RESP_BUF_SIZE];
    char contentType[64];
};

class HTTP {
public:
    explicit HTTP(ATClient& at);

    bool begin();                            // init HTTPINIT
    void end();                              // HTTPTERM

    // Simple GET / POST / PUT / DELETE
    bool get (const char* url, HTTPResponse& resp,
              uint32_t timeoutMs = AT_HTTP_TIMEOUT);

    bool post(const char* url,
              const char* body, size_t bodyLen,
              const char* contentType,
              HTTPResponse& resp,
              uint32_t timeoutMs = AT_HTTP_TIMEOUT);

    bool put (const char* url,
              const char* body, size_t bodyLen,
              const char* contentType,
              HTTPResponse& resp,
              uint32_t timeoutMs = AT_HTTP_TIMEOUT);

    bool del (const char* url, HTTPResponse& resp,
              uint32_t timeoutMs = AT_HTTP_TIMEOUT);

    // Custom header (call before get/post)
    bool addHeader(const char* key, const char* value);
    void clearHeaders();

    // SSL/TLS
    bool enableSSL(bool verify = false);

private:
    ATClient& _at;
    bool _sslEnabled = false;

    struct Header { char key[48]; char value[128]; };
    static constexpr int MAX_HEADERS = 8;
    Header _headers[MAX_HEADERS];
    int    _headerCount = 0;

    bool _sendRequest(const char* method, const char* url,
                      const char* body, size_t bodyLen,
                      const char* contentType,
                      HTTPResponse& resp, uint32_t timeoutMs);
};
