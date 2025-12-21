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
    while (true) {
        memset(buffer, 0, 1024);
        ssize_t bytes_read = recv(client_fd, buffer, 1024, 0);
        if (bytes_read <= 0) break;

        string query = trim(string(buffer));
        if (query == "exit" || query == "quit") break;

        stringstream ss;
        engine.execute(query, ss);
        string response = ss.str();
        send(client_fd, response.c_str(), response.size(), 0);
    }
    close(client_fd);
}

int main() {
    try {
        DatabaseSchema schema = SchemaManager::loadSchema("schema.json");
        SchemaManager::initDirectories(schema);
        QueryEngine engine(schema);

        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) throw runtime_error("Socket creation failed");

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(7432);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
            throw runtime_error("Bind failed");

        if (listen(server_fd, 3) < 0)
            throw runtime_error("Listen failed");

        cout << "Server started on port 7432" << endl;

        while (true) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            if (client_fd < 0) continue;
            thread(handle_client, client_fd, ref(engine)).detach();
        }
    } catch (exception& e) {
        cout << "Fatal: " << e.what() << endl;
    }
    return 0;
}