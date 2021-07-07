cmake_minimum_required(VERSION 3.0.0)
project(BullyAlgo VERSION 0.1.0)

add_executable(BullyAlgoRoutingClient src/Message.cpp src/Router.cpp src/RoutingClient.cpp src/TCPClient.cpp src/RoutingClientMain.cpp)
add_executable(BullyAlgoRoutingServer src/Message.cpp src/Router.cpp src/RoutingServer.cpp src/TCPServer.cpp src/RoutingServerMain.cpp)

add_executable(BullyAlgoNode src/Message.cpp src/Node.cpp src/Router.cpp src/RoutingClient.cpp src/TCPClient.cpp src/TCPServer.cpp src/NodeMain.cpp)
