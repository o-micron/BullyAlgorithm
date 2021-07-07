#pragma once

#include "Router.h"

#include <WinSock2.h>
#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

#define ROUTING_SERVER_MAX_LOG_COUNT 7

struct TCPServer;

/// Responsible for holding a table of all available nodes of the network
/// Provides new nodes the info needed about the current state of the network
/// This is a singleton class because we don't need more than one instance
class RoutingServer
{
  public:
    static RoutingServer& instance();
    void                  operator=(RoutingServer const&) = delete;
    RoutingServer(RoutingServer const&)                   = delete;

    /// starts a server asynchronously and loops printing table of network nodes while waiting for connections
    void runServerAsyncAndWaitForConnections();

  private:
    RoutingServer();
    ~RoutingServer();

    void        onError(std::string errorMessage, time_t timestamp);
    void        onAccept(SOCKET socket, time_t timestamp);
    std::string onRequest(std::string requestStr, time_t timestamp);
    void        onClose(time_t timestamp);
    bool        needsToShutdown(time_t timestamp);

    uint32_t ipPortComboExists(std::string ip, std::string port);
    void     logTrace(std::string text);

    static uint32_t                                       _uuid;
    TCPServer*                                            _server;
    std::array<NodeInfo, MAX_ROUTING_SERVER_NODES>        nodes;
    std::set<size_t>                                      nodesFreeSlots;
    std::set<size_t>                                      nodesBusySlots;
    std::map<std::string, std::vector<size_t>>            nodes_ip_indexer;
    std::map<std::string, std::vector<size_t>>            nodes_port_indexer;
    std::map<uint32_t, size_t>                            nodes_uuid_indexer;
    std::array<std::string, ROUTING_SERVER_MAX_LOG_COUNT> _logs;
    uint32_t                                              _logsIndex;
};
