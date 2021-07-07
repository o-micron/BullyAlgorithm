#include "../include/BullyAlgo/RoutingClient.h"
#include "../include/BullyAlgo/TCPClient.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <winsock2.h>

#include <ws2tcpip.h>

#include <assert.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

RoutingClient::RoutingClient() { _client = new TCPClient(); }

RoutingClient::~RoutingClient() { delete _client; }

RoutingClient&
RoutingClient::instance()
{
    static RoutingClient instance;
    return instance;
}

RESPONSE_CODE
RoutingClient::sendPingRequest(NodeInfo info)
{
    std::stringstream requestStream;
    requestStream << ROUTER_REQUEST_CODE_PING << " " << info.ip << " " << info.port << " " << 0;
    std::string requestStr  = requestStream.str();
    std::string responseStr = "";
    std::string errorStr    = "";
    _client->sendMessage(ROUTING_SERVER_IP, ROUTING_SERVER_PORT, requestStr, responseStr, errorStr);
    if (responseStr.size() >= 1) {
        RESPONSE_CODE code = responseStr[0];
        return code;
    } else {
        return ROUTER_RESPONSE_CODE_ERROR_BAD_REQUEST;
    }
}

RESPONSE_CODE
RoutingClient::sendRegisterRequest(NodeInfo info, uint32_t& nodeId)
{
    std::stringstream requestStream;
    requestStream << ROUTER_REQUEST_CODE_REGISTER << " " << info.ip << " " << info.port << " " << nodeId;
    std::string requestStr  = requestStream.str();
    std::string responseStr = "";
    std::string errorStr    = "";
    _client->sendMessage(ROUTING_SERVER_IP, ROUTING_SERVER_PORT, requestStr, responseStr, errorStr);
    if (!responseStr.empty()) {
        std::stringstream responseStream(responseStr);
        RESPONSE_CODE     code;
        responseStream >> code;
        if (code == ROUTER_RESPONSE_CODE_REGISTERED) {
            responseStream >> nodeId;
            return code;
        }
        return ROUTER_RESPONSE_CODE_ERROR_BAD_REQUEST;
    } else {
        return ROUTER_RESPONSE_CODE_ERROR_BAD_REQUEST;
    }
}

RESPONSE_CODE
RoutingClient::sendUnregisterRequest(NodeInfo info)
{
    std::stringstream requestStream;
    requestStream << ROUTER_REQUEST_CODE_UNREGISTER << " " << info.ip << " " << info.port << " " << info.uuid;
    std::string requestStr  = requestStream.str();
    std::string responseStr = "";
    std::string errorStr    = "";
    _client->sendMessage(ROUTING_SERVER_IP, ROUTING_SERVER_PORT, requestStr, responseStr, errorStr);
    if (responseStr.size() >= 1) {
        RESPONSE_CODE code = responseStr[0];
        return code;
    } else {
        return ROUTER_RESPONSE_CODE_ERROR_BAD_REQUEST;
    }
}

RESPONSE_CODE
RoutingClient::sendGetNodesRequest(NodeInfo info, std::set<NodeInfo>& nodes)
{
    std::stringstream requestStream;
    requestStream << ROUTER_REQUEST_CODE_GET_NODES << " " << info.ip << " " << info.port << " " << 0;
    std::string requestStr  = requestStream.str();
    std::string responseStr = "";
    std::string errorStr    = "";
    _client->sendMessage(ROUTING_SERVER_IP, ROUTING_SERVER_PORT, requestStr, responseStr, errorStr);
    if (responseStr.size() >= 1) {
        std::stringstream responseStream(responseStr);
        RESPONSE_CODE     code;
        responseStream >> code;
        if (code == ROUTER_RESPONSE_CODE_FETCH_COMPLETE) {
            size_t itemsCount;
            responseStream >> itemsCount;
            for (size_t i = 0; i < itemsCount; ++i) {
                NodeInfo info;
                responseStream >> info;
                nodes.insert(info);
            }
        }
        return code;
    } else {
        return ROUTER_RESPONSE_CODE_ERROR_BAD_REQUEST;
    }
}