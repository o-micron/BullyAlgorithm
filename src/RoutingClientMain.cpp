#include "../include/BullyAlgo/RoutingClient.h"

#include <iostream>

int
main(int argc, char** argv)
{
    if (argc < 2) {
        printf("usage RoutingClient.exe <PORT NUMBER>\n");
        return -1;
    }
    NodeInfo info = {};
#define CLIENT_IP "127.0.0.1"
    memcpy(info.ip, CLIENT_IP, sizeof(CLIENT_IP));
    memcpy(info.port, argv[1], strlen(argv[1]));
    info.uuid = 0;

    {
        // printf("\nSENDING A PING REQUEST\n");
        RoutingClient::instance().sendPingRequest(info);
    }
    {
        // printf("\nSENDING A REGISTER REQUEST\n");
        RoutingClient::instance().sendRegisterRequest(info, info.uuid);
        // printf("[REGISTERED NODE]    IP: %s\n", info.ip);
        // printf("                   PORT: %s\n", info.port);
        // printf("                   UUID: %lu\n\n", info.uuid);
        if (info.uuid == 0) { return -1; }
    }
    {
        // printf("\nSENDING A GET NODES REQUEST\n");
        std::set<NodeInfo> nodes;
        RESPONSE_CODE      code = RoutingClient::instance().sendGetNodesRequest(info, nodes);
        if (code != ROUTER_RESPONSE_CODE_ERROR_BAD_REQUEST) {
#if 0 
            char buf[128];
            snprintf(buf, sizeof(buf), "logs_node_%lu.txt", info.uuid);
            std::ofstream file;
            file.open(buf);
            for (const auto& node : nodes) { file << "[NODE] " << node << "\n"; }
            file.close();
#endif
            // {
            //     printf("\nSENDING A UNREGISTER REQUEST\n");
            //     RoutingClient::instance().sendUnregisterRequest(info);
            // }
        } else {
            std::cout << "Get nodes Bad request " << info.uuid << "\n";
        }
    }
    return 0;
}