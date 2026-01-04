#include "http_server.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <regex>

using namespace std;

string HttpServer::parseHeader(const string& request, const string& headerName) {
    size_t pos = request.find(headerName + ":");
    if (pos == string::npos) return "";
    size_t start = pos + headerName.length() + 1;
    size_t end = request.find("\r\n", start);
    return trim(request.substr(start, end - start));
}

string HttpServer::getRequestBody(const string& request) {
    size_t pos = request.find("\r\n\r\n");
    if (pos == string::npos) return "";
    return request.substr(pos + 4);
}

void HttpServer::handleRequest(int client_fd) {
    char buffer[8192];
    memset(buffer, 0, 8192);
    ssize_t bytes_received = recv(client_fd, buffer, 8191, 0);
    if (bytes_received <= 0) {
        close(client_fd);
        return;
    }

    string request(buffer);
    string responseBody = "{\"Ошибка\": \"Неизвестная команда\"}";
    string status = "404 Not Found";

    string key = parseHeader(request, "X-USER-KEY");
    string body = getRequestBody(request);

    if (request.find("POST /user") != string::npos) {
        smatch m;
        if (regex_search(body, m, regex(R"json("username"\s*:\s*"([^"]+)")json"))) {
            responseBody = exchange.createUser(m[1].str());
            status = "200 OK";
        }
    } 
    else if (request.find("GET /balance") != string::npos) {
        responseBody = exchange.getBalance(key);
        status = "200 OK";
    } 
    else if (request.find("GET /lot") != string::npos) {
        responseBody = exchange.getLots();
        status = "200 OK";
    } 
    else if (request.find("GET /pair") != string::npos) {
        responseBody = exchange.getPairs();
        status = "200 OK";
    }
    else if (request.find("GET /order") != string::npos) {
        responseBody = exchange.getOrders();
        status = "200 OK";
    }
    else if (request.find("POST /order") != string::npos) {
        smatch m;
        if (regex_search(body, m, regex(R"json("pair_id"\s*:\s*(\d+).*?"quantity"\s*:\s*([\d\.]+).*?"price"\s*:\s*([\d\.]+).*?"type"\s*:\s*"([^"]+)")json"))) {
            responseBody = exchange.createOrder(key, stoi(m[1]), stod(m[2]), stod(m[3]), m[4]);
            status = "200 OK";
        }
    } 
    else if (request.find("DELETE /order") != string::npos) {
        smatch m;
        if (regex_search(body, m, regex(R"json("order_id"\s*:\s*(\d+))json"))) {
            responseBody = exchange.deleteOrder(key, stoi(m[1]));
            status = "200 OK";
        }
    }

    string fullResponse = "HTTP/1.1 " + status + "\r\n" +
                          "Content-Type: application/json\r\n" +
                          "Content-Length: " + to_string(responseBody.length()) + "\r\n" +
                          "Connection: close\r\n\r\n" + responseBody;
    
    send(client_fd, fullResponse.c_str(), fullResponse.length(), 0);
    close(client_fd);
}

void HttpServer::start() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw runtime_error("Не удалось привязать порт HTTP сервера");
    }
    listen(server_fd, 100);
    cout << "HTTP Сервер запущен на порту " << port << endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd >= 0) {
            thread(&HttpServer::handleRequest, this, client_fd).detach();
        }
    }
}
