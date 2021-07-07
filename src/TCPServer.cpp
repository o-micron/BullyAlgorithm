#include "../include/BullyAlgo/TCPServer.h"
#include "../include/BullyAlgo/Router.h"

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <winsock2.h>

#include <ws2tcpip.h>

#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

TCPServer::~TCPServer() { _t.join(); }

void
TCPServer::runAsync(std::string       ip,
                    std::string       port,
                    OnErrorFn         errorHandler,
                    OnRequestFn       requestHandler,
                    OnAcceptFn        acceptHandler,
                    OnCloseFn         closeHandler,
                    NeedsToShutdownFn NeedsToShutdownSignal)
{
    _t = std::thread([=]() {
        WSADATA wsaData;
        int     iResult;

        SOCKET ListenSocket = INVALID_SOCKET;
        SOCKET ClientSocket = INVALID_SOCKET;

        struct addrinfo* result = NULL;
        struct addrinfo  hints;

        int  iSendResult;
        char recvbuf[MAX_SOCKET_BUFLEN];
        int  recvbuflen = MAX_SOCKET_BUFLEN;

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            time_t                                tt  = std::chrono::system_clock::to_time_t(now);
            std::stringstream                     ess;
            ess << "WSAStartup failed with error " << iResult;
            errorHandler(ess.str(), tt);
            closeHandler(tt);
            return;
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags    = AI_PASSIVE;

        // Resolve the server address and port
        iResult = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result);
        if (iResult != 0) {
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            time_t                                tt  = std::chrono::system_clock::to_time_t(now);
            std::stringstream                     ess;
            ess << "getaddrinfo failed with error " << iResult;
            errorHandler(ess.str(), tt);
            closeHandler(tt);
            WSACleanup();
            return;
        }

        // Create a SOCKET for connecting to server
        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            time_t                                tt  = std::chrono::system_clock::to_time_t(now);
            std::stringstream                     ess;
            ess << "socket failed with error " << WSAGetLastError();
            errorHandler(ess.str(), tt);
            closeHandler(tt);
            freeaddrinfo(result);
            WSACleanup();
            return;
        }

        // Setup the TCP listening socket
        iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            time_t                                tt  = std::chrono::system_clock::to_time_t(now);
            std::stringstream                     ess;
            ess << "bind failed with error " << WSAGetLastError();
            errorHandler(ess.str(), tt);
            closeHandler(tt);
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return;
        }

        freeaddrinfo(result);

        while (1) {
            iResult = listen(ListenSocket, SOMAXCONN);
            if (iResult == SOCKET_ERROR) {
                std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                time_t                                tt  = std::chrono::system_clock::to_time_t(now);
                std::stringstream                     ess;
                ess << "listen failed with error " << WSAGetLastError();
                errorHandler(ess.str(), tt);
                closeHandler(tt);
                closesocket(ListenSocket);
                WSACleanup();
                return;
            }

            // Accept a client socket
            ClientSocket = accept(ListenSocket, NULL, NULL);
            if (ClientSocket == INVALID_SOCKET) {
                std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                time_t                                tt  = std::chrono::system_clock::to_time_t(now);
                std::stringstream                     ess;
                ess << "accept failed with error " << WSAGetLastError();
                errorHandler(ess.str(), tt);
                closeHandler(tt);
                closesocket(ListenSocket);
                WSACleanup();
                return;
            }

            // Receive until the peer shuts down the connection
            do {
                iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
                if (iResult > 0) {
                    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                    time_t                                tt  = std::chrono::system_clock::to_time_t(now);
                    std::string                           requestStr(recvbuf, recvbuf + iResult);
                    std::string                           responseStr = requestHandler(requestStr, tt);
                    iSendResult = send(ClientSocket, responseStr.c_str(), static_cast<int>(responseStr.size()), 0);
                    if (iSendResult == SOCKET_ERROR) {
                        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                        time_t                                tt  = std::chrono::system_clock::to_time_t(now);
                        std::stringstream                     ess;
                        ess << "send failed with error " << WSAGetLastError();
                        errorHandler(ess.str(), tt);
                        closeHandler(tt);
                        closesocket(ClientSocket);
                        WSACleanup();
                        return;
                    }
                } else if (iResult == 0) {
                    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                    time_t                                tt  = std::chrono::system_clock::to_time_t(now);
                    closeHandler(tt);
                } else {
                    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                    time_t                                tt  = std::chrono::system_clock::to_time_t(now);
                    std::stringstream                     ess;
                    ess << "recv failed with error " << WSAGetLastError();
                    errorHandler(ess.str(), tt);
                    closeHandler(tt);
                    closesocket(ClientSocket);
                    WSACleanup();
                    return;
                }
            } while (iResult > 0);

            // shutdown the connection since we're done
            iResult = shutdown(ClientSocket, SD_SEND);
            if (iResult == SOCKET_ERROR) {
                std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
                time_t                                tt  = std::chrono::system_clock::to_time_t(now);
                std::stringstream                     ess;
                ess << "shutdown failed with error " << WSAGetLastError();
                errorHandler(ess.str(), tt);
                closeHandler(tt);
                closesocket(ClientSocket);
                WSACleanup();
                return;
            }
        }

        // cleanup
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        time_t                                tt  = std::chrono::system_clock::to_time_t(now);
        closeHandler(tt);
        closesocket(ListenSocket);
        closesocket(ClientSocket);
        WSACleanup();
    });
}