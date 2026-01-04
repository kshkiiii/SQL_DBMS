#pragma once
#include "exchange.h"
#include <netinet/in.h>

using namespace std;

class HttpServer {
    ExchangeManager& exchange;
    int port;

    void handleRequest(int client_fd);
    string parseHeader(const string& request, const string& headerName);
    string getRequestBody(const string& request);

public:
    HttpServer(ExchangeManager& em, int p) : exchange(em), port(p) {}
    void start();
};
