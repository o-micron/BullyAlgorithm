#include "../include/BullyAlgo/TCPClient.h"

#include "../include/BullyAlgo/Router.h"

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

void
TCPClient::sendMessage(std::string hostIp, std::string hostPort, std::string data, std::string& response, std::string& error)
{
    WSADATA          wsaData;
    SOCKET           ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    char             recvbuf[MAX_SOCKET_BUFLEN];
    int              iResult;
    int              recvbuflen = MAX_SOCKET_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::stringstream ess;
        ess << "WSAStartup failed with error " << iResult << "\n";
        error = ess.str();
        return;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(hostIp.c_str(), hostPort.c_str(), &hints, &result);
    if (iResult != 0) {
        std::stringstream ess;
        ess << "getaddrinfo failed with error " << iResult << "\n";
        error = ess.str();
        WSACleanup();
        return;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            std::stringstream ess;
            ess << "socket failed with error " << WSAGetLastError() << "\n";
            error = ess.str();
            WSACleanup();
            return;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        std::stringstream ess;
        ess << "Unable to connect to server\n";
        error = ess.str();
        WSACleanup();
        return;
    }

    // Send an initial buffer
    iResult = send(ConnectSocket, data.c_str(), static_cast<int>(data.size()), 0);
    if (iResult == SOCKET_ERROR) {
        std::stringstream ess;
        ess << "send failed with error " << WSAGetLastError() << "\n";
        error = ess.str();
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        std::stringstream ess;
        ess << "shutdown failed with error " << WSAGetLastError() << "\n";
        error = ess.str();
        closesocket(ConnectSocket);
        WSACleanup();
        return;
    }

    // Receive until the peer closes the connection
    do {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            response = std::string(recvbuf, recvbuf + iResult);
        } else if (iResult == 0) {
            // printf("Connection closed\n");
        } else {
            std::stringstream ess;
            ess << "recv failed with error " << WSAGetLastError() << "\n";
            error = ess.str();
        }
    } while (iResult > 0);

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
}