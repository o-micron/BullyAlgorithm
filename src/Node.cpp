#include "../include/BullyAlgo/Node.h"

#include "../include/BullyAlgo/ConsoleUi.h"
#include "../include/BullyAlgo/RoutingClient.h"
#include "../include/BullyAlgo/TCPClient.h"
#include "../include/BullyAlgo/TCPServer.h"

#include <iostream>
#include <sstream>

Node::Node(std::string port)
{
    _flagCallForElection.store(true);
    _logsIndex = 0;
#define CLIENT_IP "127.0.0.1"
    memset(_info.ip, '\0', sizeof(_info.ip));
    memset(_info.port, '\0', sizeof(_info.port));
    memcpy(_info.ip, CLIENT_IP, sizeof(CLIENT_IP));
    memcpy(_info.port, port.c_str(), port.size());
    _info.uuid = 0;

    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    time_t                                tt  = std::chrono::system_clock::to_time_t(now);
    _pingCounter.store(tt);
    _coordinatorUUID.store(_info.uuid);

    _server = new TCPServer();
    _client = new TCPClient();
}

Node::~Node()
{
    delete _client;
    delete _server;
}

void
Node::run()
{
    _serverRunning = true;

    std::string ip(_info.ip, _info.ip + strlen(_info.ip));
    std::string port(_info.port, _info.port + strlen(_info.port));

    _server->runAsync(
      ip,
      port,
      [&](std::string errorMessage, time_t timestamp) { this->onError(errorMessage, timestamp); },
      [&](std::string requestStr, time_t timestamp) -> std::string { return this->onRequest(requestStr, timestamp); },
      [&](SOCKET socket, time_t timestamp) { this->onAccept(socket, timestamp); },
      [&](time_t timestamp) { this->onClose(timestamp); },
      [&](time_t timestamp) -> bool { return this->needsToShutdown(timestamp); });

    {
        RoutingClient::instance().sendPingRequest(_info);
    }
    {
        RoutingClient::instance().sendRegisterRequest(_info, _info.uuid);
    }
    {
        RESPONSE_CODE code = RoutingClient::instance().sendGetNodesRequest(_info, _nodes);
        if (code == ROUTER_RESPONSE_CODE_ERROR_BAD_REQUEST) {
            std::stringstream lss;
            lss << "Get nodes Bad request " << _info.uuid << "\n";
            logTrace(lss.str());
            return;
        }
    }

    while (_serverRunning) {
        auto start = std::chrono::high_resolution_clock::now();
        if (_flagCallForElection.load()) {
            _flagCallForElection.store(false);
            sendCallForElection();
        }
        CLEAR_CONSOLE();
        short row = 1;
        short col = 2;
        GOTOXY(col, row);
        std::cout << "NODE INFORMATION\n";
        ++row;

        GOTOXY(col, row);
        std::cout << "UUID";
        GOTOXY(col + 25, row);
        std::cout << _info.uuid;
        ++row;

        GOTOXY(col, row);
        std::cout << "IP";
        GOTOXY(col + 25, row);
        std::cout << _info.ip;
        ++row;

        GOTOXY(col, row);
        std::cout << "PORT";
        GOTOXY(col + 25, row);
        std::cout << _info.port;
        ++row;

        GOTOXY(col, row);
        std::cout << "COORDINATOR";
        GOTOXY(col + 25, row);
        std::cout << _coordinatorUUID.load();
        ++row;

        ++row;
        short tableRow = row;

        // header
        GOTOXY(col, row);
        std::cout << "UUID";
        GOTOXY(col + 5, row);
        std::cout << "IP ADDRESS";
        GOTOXY(col + 17, row);
        std::cout << "PORT";
        std::cout << "\n";
        ++row;

        for (const auto& node : _nodes) {
            if (row % 19 == 0) {
                row = tableRow;
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
            ss << node;
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
        row = 20;
        GOTOXY(col, row);
        for (uint32_t i = 0; i < NODE_MAX_LOG_COUNT; ++i) {
            GOTOXY(col, row);
            std::cout << _logs[i];
            ++row;
        }

        // run server asynchronously
        // loop and wait for timeout
        // check for backlog if we received a ping from the coordinator
        // if coordinator drops we loop on all network nodes and we call for election
        // if everything is alright then we can look in the backlog for messages
        // messages types could be:
        //      1 - coordinator ping
        //      2 - someone calling for election
        //      3 - coordinator sending a task
        //      4 - controller remotely simulating a force shutdown

        if (_coordinatorUUID.load() <= _info.uuid) {
            // i am the coordinator let's act as one
            for (const auto& node : _nodes) {
                std::stringstream mss;
                mss << NODE_REQUEST_CODE_PING << " " << _info.ip << " " << _info.port << " " << _info.uuid;
                std::string responseStr = "";
                std::string errorStr    = "";
                _client->sendMessage(node.ip, node.port, mss.str(), responseStr, errorStr);
            }
            auto                                      finish   = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = finish - start;
            std::this_thread::sleep_for(std::chrono::seconds(2) - duration);
        } else {
            // i am a follower, act as one
            std::chrono::system_clock::time_point now                = std::chrono::system_clock::now();
            time_t                                tt                 = std::chrono::system_clock::to_time_t(now);
            time_t                                lastRegisteredPing = _pingCounter.load();
            auto                                  seconds            = difftime(tt, lastRegisteredPing);
            if (seconds > 2) {
                _pingCounter.store(tt);
                _coordinatorUUID.store(_info.uuid);
                _flagCallForElection.store(false);
                sendCallForElection();
            }
            auto                                      finish   = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = finish - start;
            std::this_thread::sleep_for(std::chrono::seconds(2) - duration);
        }
    }
}

void
Node::onError(std::string errorMessage, time_t timestamp)
{
    _serverRunning = false;
}

void
Node::onAccept(SOCKET socket, time_t timestamp)
{}

std::string
Node::onRequest(std::string requestStr, time_t timestamp)
{
    if (requestStr.empty()) return "";

    std::stringstream requestStream(requestStr);
    std::stringstream responseStream;
    REQUEST_CODE      code;
    NodeInfo          senderInfo;
    requestStream >> code >> senderInfo;

    if (_nodes.find(senderInfo) == _nodes.end()) {
        // a new node that has entered and we need to register it
        if (senderInfo.uuid != 0) { _nodes.insert(senderInfo); }
    }

    tm                utc_tm = *gmtime(&timestamp);
    std::stringstream lss;
    lss << "[REQUEST] " << utc_tm.tm_year + 1900 << "/" << utc_tm.tm_mon + 1 << "/" << utc_tm.tm_mday << " " << utc_tm.tm_hour
        << ":" << utc_tm.tm_min << ":" << utc_tm.tm_sec << " " << senderInfo.ip << ":" << senderInfo.port
        << " Bytes: " << requestStr.size() << " Text: " << requestStr;
    logTrace(lss.str());

    if (code == NODE_REQUEST_CODE_PING) {
        if (senderInfo.uuid == _coordinatorUUID) {
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            time_t                                tt  = std::chrono::system_clock::to_time_t(now);
            _pingCounter.store(tt);
            responseStream << NODE_RESPONSE_CODE_OK;
        }
    } else if (code == NODE_REQUEST_CODE_ELECTION) {
        if (senderInfo.uuid > _coordinatorUUID.load()) {
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            time_t                                tt  = std::chrono::system_clock::to_time_t(now);
            _pingCounter.store(tt);
            _coordinatorUUID.store(senderInfo.uuid);
        } else {
            _flagCallForElection.store(true);
        }
    } else {
        std::stringstream responseStream;
        responseStream << NODE_RESPONSE_CODE_OK;
    }
    return responseStream.str();
}

void
Node::onClose(time_t timestamp)
{}

bool
Node::needsToShutdown(time_t timestamp)
{
    return false;
}

void
Node::sendCallForElection()
{
    std::stringstream requestStream;
    requestStream << NODE_REQUEST_CODE_ELECTION << " " << _info;
    for (const auto& node : _nodes) {
        std::string response = "";
        std::string error    = "";
        _client->sendMessage(node.ip, node.port, requestStream.str(), response, error);
    }
}

void
Node::logTrace(std::string text)
{
    _logs[_logsIndex] = text;
    ++_logsIndex;
    _logsIndex %= NODE_MAX_LOG_COUNT;
}