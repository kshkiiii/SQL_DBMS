#include "schema.h"
#include "engine.h"
#include "exchange.h"
#include "http_server.h"

using namespace std;

int main() {
    try {
        DatabaseSchema schema = SchemaManager::loadSchema("schema.json");
        SchemaManager::initDirectories(schema);
        QueryEngine engine(schema);
        
        ExchangeManager exchange(engine, "config.json");
        exchange.setup();
        
        HttpServer server(exchange, 8080);
        server.start();
    } catch (exception& e) {
        cout << "Ошибка запуска биржи: " << e.what() << endl;
    }
    return 0;
}
