#pragma once

#include <fstream>
#include <regex>
#include <stdint.h>
#include <string>

// a single byte indicating the type of response, the first byte of the response message
// -128 --> +127
typedef char RESPONSE_CODE;

// a single byte indicating the type of request, the first byte of the request message
// -128 --> +127
typedef char REQUEST_CODE;

struct NodeInfo
{
    uint32_t uuid;
    char     ip[16];
    char     port[5];

    bool isValid()
    {
#define IP_REGEX_VALIDATOR   "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"
#define PORT_REGEX_VALIDATOR "^[5-9][0-9]{3}$"

        if (uuid == 0) return false;
        if (!std::regex_match(std::string(ip), std::regex(IP_REGEX_VALIDATOR))) return false;
        if (!std::regex_match(std::string(port), std::regex(PORT_REGEX_VALIDATOR))) return false;
        return true;
    }
};

bool
operator<(const NodeInfo& lhs, const NodeInfo& rhs);

std::ostream&
operator<<(std::ostream& os, const NodeInfo& info);

std::istream&
operator>>(std::istream& is, NodeInfo& info);