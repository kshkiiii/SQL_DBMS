#include "schema.h"
#include "engine.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <cstring>

using namespace std;

void handle_client(int client_fd, QueryEngine& engine) {
    char buffer[1024];
    memset(buffer, 0, 1024);
    ssize_t bytes_read = recv(client_fd, buffer, 1024, 0);
    if (bytes_read > 0) {
        string query = trim(string(buffer));
        stringstream ss;
        engine.execute(query, ss);
        string response = ss.str();
        if (response.empty()) response = "OK\n";
        send(client_fd, response.c_str(), response.size(), 0);
    }
    close(client_fd);
}

int main() {
    DatabaseSchema schema = SchemaManager::loadSchema("schema.json");
    SchemaManager::initDirectories(schema);
    QueryEngine engine(schema);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(7432);
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);
    cout << "СУБД запущена на порту 7432" << endl;
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd >= 0) thread(handle_client, client_fd, ref(engine)).detach();
    }
    return 0;
}
