#include "../include/BullyAlgo/RoutingServer.h"

int
main(int argc, char** argv)
{
    RoutingServer::instance().runServerAsyncAndWaitForConnections();
    return 0;
}