#pragma once
#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <string>

// I'm proud of this useless script.. :DD 

class HTTPServer {
public:
    HTTPServer(int port = 4141);
    ~HTTPServer();

    void start();

private:
    int serverSocket;
    int port;

    void initSocket();

    void handleClient(int clientSocket);

    void sendResponse(int clientSocket, const std::string& content);
};

#endif 