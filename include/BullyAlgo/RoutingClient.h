#pragma once

#include "Router.h"

#include <set>
#include <string>

struct TCPClient;

/// Responsible for sending requests to the routing server
/// This is a singleton class because we don't need more than one instance
class RoutingClient
{
  public:
    static RoutingClient& instance();
    void                  operator=(RoutingClient const&) = delete;
    RoutingClient(RoutingClient const&)                   = delete;

    /// Ping the routing server to know if it is alive (we assume it will always be alive)
    RESPONSE_CODE sendPingRequest(NodeInfo info);

    /// Register the new node to the routing server by filling only the ip and port fields
    /// If no (ip + port) already registered with the same values, a new uuid is returned from the server at nodeId
    RESPONSE_CODE sendRegisterRequest(NodeInfo info, uint32_t& nodeId);

    /// Unregister a node from the routing server
    /// This could be called because a node crashed
    RESPONSE_CODE sendUnregisterRequest(NodeInfo info);

    /// Request all available nodes from the routing server
    /// The nodes are returned in a std::set called nodes
    RESPONSE_CODE sendGetNodesRequest(NodeInfo info, std::set<NodeInfo>& nodes);

  private:
    RoutingClient();
    ~RoutingClient();
    TCPClient* _client;
};
