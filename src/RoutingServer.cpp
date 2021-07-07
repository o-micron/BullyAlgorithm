#include "../include/BullyAlgo/RoutingServer.h"
#include "../include/BullyAlgo/ConsoleUi.h"
#include "../include/BullyAlgo/TCPServer.h"

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <winsock2.h>

#include <ws2tcpip.h>

#include <algorithm>
#include <ctype.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

uint32_t RoutingServer::_uuid = 0;

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

RoutingServer::RoutingServer()
{
    _logsIndex = 0;
    _server    = new TCPServer();
    for (size_t i = 0; i < MAX_ROUTING_SERVER_NODES; ++i) {
        nodesFreeSlots.insert(i);
        nodes[i].uuid = 0;
    }
}

RoutingServer::~RoutingServer() { delete _server; }

RoutingServer&
RoutingServer::instance()
{
    static RoutingServer instance;
    return instance;
}

void
RoutingServer::runServerAsyncAndWaitForConnections()
{
    _server->runAsync(
      ROUTING_SERVER_IP,
      ROUTING_SERVER_PORT,
      [&](std::string errorMessage, time_t timestamp) { this->onError(errorMessage, timestamp); },
      [&](std::string requestStr, time_t timestamp) -> std::string { return this->onRequest(requestStr, timestamp); },
      [&](SOCKET socket, time_t timestamp) { this->onAccept(socket, timestamp); },
      [&](time_t timestamp) { this->onClose(timestamp); },
      [&](time_t timestamp) -> bool { return this->needsToShutdown(timestamp); });

    while (1) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        CLEAR_CONSOLE();
        short row = 1;
        short col = 2;
        GOTOXY(col, row);
        std::cout << "NETWORK ROUTING TABLE\t\tCONNECT TO " << ROUTING_SERVER_IP << ":" << ROUTING_SERVER_PORT
                  << " TO ENTER THE NETWORK\n";
        row = 3;
        GOTOXY(col, row);
        std::cout << "UUID";
        GOTOXY(col + 5, row);
        std::cout << "IP ADDRESS";
        GOTOXY(col + 17, row);
        std::cout << "PORT";
        std::cout << "\n";
        ++row;
        for (const auto& nodeIndex : nodesBusySlots) {
            if (row % 21 == 0) {
                row = 3;
                col += 30;
                GOTOXY(col, row);
                std::cout << "UUID";
                GOTOXY(col + 5, row);
                std::cout << "IP ADDRESS";
                GOTOXY(col + 17, row);
                std::cout << "PORT";
                std::cout << "\n";
                ++row;
            }
            std::stringstream ss;
            ss << nodes[nodeIndex];
            std::string ip, port, uuid;
            ss >> ip >> port >> uuid;
            GOTOXY(col, row);
            std::cout << uuid;
            GOTOXY(col + 5, row);
            std::cout << ip;
            GOTOXY(col + 17, row);
            std::cout << port << "\n";
            ++row;
        }

        col = 1;
        row = 22;
        GOTOXY(col, row);
        for (uint32_t i = 0; i < ROUTING_SERVER_MAX_LOG_COUNT; ++i) {
            GOTOXY(col, row);
            std::cout << _logs[i];
            ++row;
        }
    }
}

void
RoutingServer::onError(std::string errorMessage, time_t timestamp)
{
    std::stringstream ss;
    ss << "[ERROR][ROUTING_SERVER] " << errorMessage << "\n";
    logTrace(ss.str());
}

void
RoutingServer::onAccept(SOCKET socket, time_t timestamp)
{}

std::string
RoutingServer::onRequest(std::string requestStr, time_t timestamp)
{
    std::stringstream requestStream(requestStr);
    std::stringstream responseStream;
    std::stringstream logss;

    if (requestStr.size() >= 1) {
        REQUEST_CODE requestCode;
        NodeInfo     senderInfo;
        requestStream >> requestCode;
        requestStream >> senderInfo;

        tm utc_tm = *gmtime(&timestamp);
        logss << "[REQUEST] " << utc_tm.tm_year + 1900 << "/" << utc_tm.tm_mon + 1 << "/" << utc_tm.tm_mday << " "
              << utc_tm.tm_hour << ":" << utc_tm.tm_min << ":" << utc_tm.tm_sec << " " << senderInfo.ip << ":" << senderInfo.port
              << " Bytes: " << requestStr.size() << " Text: " << requestStr;

        if (requestCode == ROUTER_REQUEST_CODE_PING) {
            logss << " [PING]";
            responseStream << ROUTER_RESPONSE_CODE_PONG;
        } else if (requestCode == ROUTER_REQUEST_CODE_REGISTER) {
            logss << " [REGISTER]";
            // check if there is free slots available to add a new node in the network
            // also check for existing combination of ip and port
            bool valid = !nodesFreeSlots.empty();

            // NOTE: not really used at the moment
            // If a node crashes and reconnects with the same ip and port it will get the same uuid as before ..
            // valid &= ;
            if (valid) {
                // check if an ip and port were already registered
                // in this case use the old uuid
                uint32_t oldUUID = ipPortComboExists(senderInfo.ip, senderInfo.port);
                if (oldUUID != 0) {
                    senderInfo.uuid = oldUUID;
                    responseStream << ROUTER_RESPONSE_CODE_REGISTERED << " " << senderInfo.uuid;
                } else {
                    // no entry with the same ip and port values, add a new entry
                    senderInfo.uuid = ++_uuid;
                    if (senderInfo.isValid()) {
                        auto   nodeFreeSlotIt    = nodesFreeSlots.begin();
                        size_t freeNodeSlotIndex = *nodeFreeSlotIt;
                        nodesFreeSlots.erase(nodeFreeSlotIt);
                        nodesBusySlots.insert(freeNodeSlotIndex);
                        nodes[freeNodeSlotIndex] = senderInfo;
                        nodes_ip_indexer[senderInfo.ip].push_back(freeNodeSlotIndex);
                        nodes_port_indexer[senderInfo.port].push_back(freeNodeSlotIndex);
                        nodes_uuid_indexer[_uuid] = freeNodeSlotIndex;
                        responseStream << ROUTER_RESPONSE_CODE_REGISTERED << " " << senderInfo.uuid;
                    } else {
                        responseStream << ROUTER_RESPONSE_CODE_ERROR_BAD_REQUEST;
                    }
                }
            } else {
                responseStream << ROUTER_RESPONSE_CODE_ERROR_DUPLICATE_IP_PORT;
            }

        } else if (requestCode == ROUTER_REQUEST_CODE_UNREGISTER) {
            // NOTE: not really used at the moment
            logss << " [UNREGISTER]";
            auto uuidIt = nodes_uuid_indexer.find(_uuid);
            if (uuidIt != nodes_uuid_indexer.end()) {
                size_t nodeSlotIndex = uuidIt->second;
                auto   ipIt          = nodes_ip_indexer.find(nodes[nodeSlotIndex].ip);
                auto   portIt        = nodes_port_indexer.find(nodes[nodeSlotIndex].port);
                if (ipIt != nodes_ip_indexer.end() && portIt != nodes_port_indexer.end()) {
                    nodes_ip_indexer.erase(ipIt->first);
                    nodes_port_indexer.erase(portIt->first);
                    nodes_uuid_indexer.erase(uuidIt->first);
                    nodesFreeSlots.insert(nodeSlotIndex);
                    nodesBusySlots.erase(nodeSlotIndex);
                    nodes[nodeSlotIndex].uuid = 0;
                    responseStream << ROUTER_RESPONSE_CODE_UNREGISTERED;
                } else {
                    responseStream << ROUTER_RESPONSE_CODE_ERROR_NOT_REGISTERED;
                }
            } else {
                responseStream << ROUTER_RESPONSE_CODE_ERROR_NOT_REGISTERED;
            }
        } else if (requestCode == ROUTER_REQUEST_CODE_GET_NODES) {
            logss << " [GET NODES]";
            responseStream << ROUTER_RESPONSE_CODE_FETCH_COMPLETE << " " << nodesBusySlots.size() << " ";
            for (const auto& nodeIndex : nodesBusySlots) { responseStream << nodes[nodeIndex]; }
        } else {
            logss << " [BAD REQUEST]";
            responseStream << ROUTER_RESPONSE_CODE_ERROR_BAD_REQUEST;
        }

        logTrace(logss.str());
        std::string responseStr = responseStream.str();
        return responseStr;
    }

    return "";
}

void
RoutingServer::onClose(time_t timestamp)
{}

bool
RoutingServer::needsToShutdown(time_t timestamp)
{
    return false;
}

uint32_t
RoutingServer::ipPortComboExists(std::string ip, std::string port)
{
    auto ipIt = nodes_ip_indexer.find(ip);
    if (ipIt == nodes_ip_indexer.end()) { return 0; }

    auto portIt = nodes_port_indexer.find(port);
    if (portIt == nodes_port_indexer.end()) { return 0; }

    std::vector<size_t> commonIndices(ipIt->second.size() + portIt->second.size());

    // iterator to store return type
    std::vector<size_t>::iterator it, end;

    end = std::set_intersection(
      ipIt->second.begin(), ipIt->second.end(), portIt->second.begin(), portIt->second.end(), commonIndices.begin());

    for (it = commonIndices.begin(); it != end; it++) {
        if (nodes[*it].ip == ip && nodes[*it].port == port) { return nodes[*it].uuid; }
    }

    return 0;
}

void
RoutingServer::logTrace(std::string text)
{
    _logs[_logsIndex] = text;
    ++_logsIndex;
    _logsIndex %= ROUTING_SERVER_MAX_LOG_COUNT;
}