#pragma once
#include "engine.h"
#include <vector>
#include <string>
#include <random>

using namespace std;

struct UserInfo {
    string id;
    string key;
};

class ExchangeManager {
    QueryEngine& engine;
    string configPath;
    vector<string> availableLots;

    void loadConfig();
    void initLotsAndPairs();
    string executeQuery(string query);
    string generateKey();
    UserInfo getUserByKey(string key);
    void updateBalance(string userId, string lotId, double delta);

public:
    ExchangeManager(QueryEngine& e, const string& cfg) : engine(e), configPath(cfg) {}
    void setup();
    
    string createUser(string username);
    string getBalance(string key);
    string getLots();
    string getPairs();
    string getOrders();
    string createOrder(string key, int pairId, double quantity, double price, string type);
    string deleteOrder(string key, int orderId);
};
