#include "HTTP.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

HTTP::HTTP(ATClient& at) : _at(at), _headerCount(0) {}

bool HTTP::begin() {
    return _at.sendCmd("AT+HTTPINIT") == ATResult::OK;
}

void HTTP::end() {
    _at.sendCmd("AT+HTTPTERM");
}

bool HTTP::enableSSL(bool verify) {
    _sslEnabled = true;
    char cmd[40];
    snprintf(cmd, sizeof(cmd), "AT+HTTPSSL=1");
    if (_at.sendCmd(cmd) != ATResult::OK) return false;
    if (!verify) {
        _at.sendCmd("AT+CSSLCFG=\"ignorlocaltime\",0,1");
    }
    return true;
}

bool HTTP::addHeader(const char* key, const char* value) {
    if (_headerCount >= MAX_HEADERS) return false;
    strncpy(_headers[_headerCount].key,   key,   sizeof(_headers[0].key)   - 1);
    strncpy(_headers[_headerCount].value, value, sizeof(_headers[0].value) - 1);
    _headerCount++;
    return true;
}

void HTTP::clearHeaders() {
    _headerCount = 0;
}

bool HTTP::get(const char* url, HTTPResponse& resp, uint32_t timeoutMs) {
    return _sendRequest("GET", url, nullptr, 0, nullptr, resp, timeoutMs);
}

bool HTTP::post(const char* url, const char* body, size_t bodyLen,
                       const char* contentType, HTTPResponse& resp, uint32_t timeoutMs) {
    return _sendRequest("POST", url, body, bodyLen, contentType, resp, timeoutMs);
}

bool HTTP::put(const char* url, const char* body, size_t bodyLen,
                      const char* contentType, HTTPResponse& resp, uint32_t timeoutMs) {
    return _sendRequest("PUT", url, body, bodyLen, contentType, resp, timeoutMs);
}

bool HTTP::del(const char* url, HTTPResponse& resp, uint32_t timeoutMs) {
    return _sendRequest("DELETE", url, nullptr, 0, nullptr, resp, timeoutMs);
}

bool HTTP::_sendRequest(const char* method, const char* url,
                                const char* body, size_t bodyLen,
                                const char* contentType,
                                HTTPResponse& resp, uint32_t timeoutMs) {
    memset(&resp, 0, sizeof(resp));
    char cmd[HTTP_RESP_BUF_SIZE];

    // URL
    snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"URL\",\"%s\"", url);
    if (_at.sendCmd(cmd) != ATResult::OK) return false;

    // Content-type
    if (contentType) {
        snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"CONTENT\",\"%s\"", contentType);
        _at.sendCmd(cmd);
    }

    // Custom headers
    for (int i = 0; i < _headerCount; i++) {
        snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"USERDATA\",\"%s: %s\\r\\n\"",
                 _headers[i].key, _headers[i].value);
        _at.sendCmd(cmd);
    }

    // Send body if present
    if (body && bodyLen > 0) {
        snprintf(cmd, sizeof(cmd), "AT+HTTPDATA=%u,%u", (unsigned)bodyLen, (unsigned)timeoutMs);
        if (_at.sendCmd(cmd, "DOWNLOAD", AT_DEFAULT_TIMEOUT) != ATResult::OK) return false;
        _at.sendRaw((const uint8_t*)body, bodyLen, "OK", AT_DEFAULT_TIMEOUT);
    }

    // Action: 0=GET, 1=POST, 2=HEAD, 3=DELETE, 4=PUT
    int action = 0;
    if      (strcmp(method, "POST")   == 0) action = 1;
    else if (strcmp(method, "DELETE") == 0) action = 3;
    else if (strcmp(method, "PUT")    == 0) action = 4;

    snprintf(cmd, sizeof(cmd), "AT+HTTPACTION=%d", action);
    char actionResp[64];
    if (_at.sendCmd(cmd, "+HTTPACTION:", timeoutMs, actionResp, sizeof(actionResp)) != ATResult::OK)
        return false;

    // +HTTPACTION: <method>,<status>,<datalen>
    const char* p = strstr(actionResp, "+HTTPACTION:");
    if (p) {
        int m, status, dlen;
        if (sscanf(p, "+HTTPACTION: %d,%d,%d", &m, &status, &dlen) == 3) {
            resp.statusCode = status;
            resp.bodyLen    = dlen;
        }
    }

    // Read response body
    if (resp.bodyLen > 0) {
        snprintf(cmd, sizeof(cmd), "AT+HTTPREAD=0,%d",
                 (resp.bodyLen < HTTP_RESP_BUF_SIZE - 1) ? resp.bodyLen : HTTP_RESP_BUF_SIZE - 1);
        _at.sendCmd(cmd, "OK", AT_DEFAULT_TIMEOUT, resp.body, sizeof(resp.body));
    }

    return resp.statusCode >= 200 && resp.statusCode < 400;
}
