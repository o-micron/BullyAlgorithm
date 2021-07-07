#pragma once

#include "Message.h"

#include <WinSock2.h>
#include <array>
#include <set>

#define NODE_MAX_LOG_COUNT 9

struct TCPClient;
struct TCPServer;

// the message went through to the other side
static const RESPONSE_CODE NODE_RESPONSE_CODE_OK = 1;

// the message failed to get sent to server (probably node server is down in that case)
static const RESPONSE_CODE NODE_RESPONSE_CODE_ERROR_SERVER_UNREACHABLE = -1;

static const REQUEST_CODE NODE_REQUEST_CODE_PING     = 1;
static const REQUEST_CODE NODE_REQUEST_CODE_ELECTION = 2;
static const REQUEST_CODE NODE_REQUEST_CODE_TASK     = 3;

struct NodeMessage
{
    union
    {
        struct
        {
            RESPONSE_CODE responseCode;
        };
        struct
        {
            REQUEST_CODE requestCode;
        };
    };
};

// # ####/##/## ##:##:## ###.###.###.### ####

struct Node
{
    Node(std::string port);
    ~Node();

    /// runs a new node server and client
    void run();

  private:
    void        onError(std::string errorMessage, time_t timestamp);
    void        onAccept(SOCKET socket, time_t timestamp);
    std::string onRequest(std::string requestStr, time_t timestamp);
    void        onClose(time_t timestamp);
    bool        needsToShutdown(time_t timestamp);

    void sendCallForElection();

    void logTrace(std::string text);

    TCPServer*                                  _server;
    TCPClient*                                  _client;
    NodeInfo                                    _info;
    std::atomic<uint32_t>                       _coordinatorUUID;
    std::set<NodeInfo>                          _nodes;
    bool                                        _serverRunning;
    std::atomic<bool>                           _flagCallForElection;
    std::atomic<time_t>                         _pingCounter;
    std::array<std::string, NODE_MAX_LOG_COUNT> _logs;
    uint32_t                                    _logsIndex;
};
