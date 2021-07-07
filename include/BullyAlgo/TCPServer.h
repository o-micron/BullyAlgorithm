#pragma once

#include <functional>
#include <string>
#include <thread>
#include <winsock2.h>

typedef std::function<void(std::string errorMessage, time_t timestamp)>      OnErrorFn;
typedef std::function<void(SOCKET socket, time_t timestamp)>                 OnAcceptFn;
typedef std::function<std::string(std::string requestStr, time_t timestamp)> OnRequestFn;
typedef std::function<void(time_t timestamp)>                                OnCloseFn;
typedef std::function<bool(time_t timestamp)>                                NeedsToShutdownFn;

struct TCPServer
{
    TCPServer() = default;
    ~TCPServer();
    void runAsync(std::string       ip,
                  std::string       port,
                  OnErrorFn         errorHandler,
                  OnRequestFn       requestHandler,
                  OnAcceptFn        acceptHandler,
                  OnCloseFn         closeHandler,
                  NeedsToShutdownFn NeedsToShutdownSignal);

  private:
    std::thread _t;
};
