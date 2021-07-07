#pragma once

#include <string>

struct TCPClient
{
    TCPClient()  = default;
    ~TCPClient() = default;

    void sendMessage(std::string hostIp, std::string hostPort, std::string data, std::string& response, std::string& error);
};
