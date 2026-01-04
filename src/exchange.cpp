#include "exchange.h"
#include <fstream>
#include <regex>
#include <iomanip>

using namespace std;

void ExchangeManager::loadConfig() {
    ifstream f(configPath);
    if (!f.is_open()) throw runtime_error("Файл конфигурации не найден");
    stringstream buffer;
    buffer << f.rdbuf();
    string content = buffer.str();
    regex lotRegex(R"json(\"([A-Z0-9]+)\")json");
    auto it = sregex_iterator(content.begin(), content.end(), lotRegex);
    for (; it != sregex_iterator(); ++it) {
        availableLots.push_back((*it)[1].str());
    }
}

string ExchangeManager::executeQuery(string query) {
    cout << "[LOG] Запрос к БД: " << query << endl;
    stringstream ss;
    engine.execute(query, ss);
    string res = ss.str();
    if (!res.empty()) cout << "[LOG] Ответ БД: " << trim(res) << endl;
    return res;
}

void ExchangeManager::initLotsAndPairs() {
    for (const auto& lotName : availableLots) {
        executeQuery("INSERT INTO lot VALUES ('" + lotName + "')");
    }
    for (size_t i = 1; i <= availableLots.size(); ++i) {
        for (size_t j = 1; j <= availableLots.size(); ++j) {
            if (i == j) continue;
            executeQuery("INSERT INTO pair VALUES ('" + to_string(i) + "', '" + to_string(j) + "')");
        }
    }
}

void ExchangeManager::setup() {
    loadConfig();
    initLotsAndPairs();
    cout << "Биржа успешно инициализирована. Загружено лотов: " << availableLots.size() << endl;
}

string ExchangeManager::generateKey() {
    string chars = "0123456789abcdef";
    string key = "";
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, (int)chars.size() - 1);
    for (int i = 0; i < 32; ++i) key += chars[dis(gen)];
    return key;
}

string ExchangeManager::createUser(string username) {
    string key = generateKey();
    executeQuery("INSERT INTO user VALUES ('" + username + "', '" + key + "')");
    
    stringstream ss;
    engine.execute("SELECT user.user_pk FROM user WHERE user.key = '" + key + "'", ss);
    string userId = trim(ss.str());

    for (size_t i = 1; i <= availableLots.size(); ++i) {
        executeQuery("INSERT INTO user_lot VALUES ('" + userId + "', '" + to_string(i) + "', '1000')");
    }

    return "{\n  \"key\": \"" + key + "\"\n}";
}

string ExchangeManager::getBalance(string key) {
    UserInfo user = getUserByKey(key);
    if (user.id.empty() || user.id == "NULL") return "{\"Ошибка\": \"Пользователь не найден\"}";

    stringstream ss_bal;
    engine.execute("SELECT user_lot.lot_id, user_lot.quantity FROM user_lot WHERE user_lot.user_id = '" + user.id + "'", ss_bal);
    
    string line;
    string result = "[\n";
    bool first = true;
    while (getline(ss_bal, line)) {
        if (line.empty()) continue;
        auto parts = split(line, ',');
        if (parts.size() < 2) continue;
        if (!first) result += ",\n";
        result += "  {\"lot_id\": " + parts[0] + ", \"quantity\": " + parts[1] + "}";
        first = false;
    }
    result += "\n]";
    return result;
}

string ExchangeManager::getLots() {
    stringstream ss;
    engine.execute("SELECT * FROM lot", ss);
    string line, result = "[\n";
    bool first = true;
    while (getline(ss, line)) {
        if (line.empty()) continue;
        auto parts = split(line, ',');
        if (!first) result += ",\n";
        result += "  {\"lot_id\": " + parts[0] + ", \"name\": \"" + parts[1] + "\"}";
        first = false;
    }
    result += "\n]";
    return result;
}

string ExchangeManager::getPairs() {
    stringstream ss;
    engine.execute("SELECT * FROM pair", ss);
    string line, result = "[\n";
    bool first = true;
    while (getline(ss, line)) {
        if (line.empty()) continue;
        auto parts = split(line, ',');
        if (!first) result += ",\n";
        result += "  {\"pair_id\": " + parts[0] + ", \"sale_lot_id\": " + parts[1] + ", \"buy_lot_id\": " + parts[2] + "}";
        first = false;
    }
    result += "\n]";
    return result;
}

string ExchangeManager::getOrders() {
    stringstream ss;
    engine.execute("SELECT * FROM order", ss);
    string line, result = "[\n";
    bool first = true;
    while (getline(ss, line)) {
        if (line.empty()) continue;
        auto parts = split(line, ',');
        if (!first) result += ",\n";
        result += "  {\"order_id\": " + parts[0] + ", \"user_id\": " + parts[1] + ", \"pair_id\": " + parts[2] + ", \"quantity\": " + parts[3] + ", \"price\": " + parts[4] + ", \"type\": \"" + parts[5] + "\", \"closed\": \"" + (parts.size() > 6 ? parts[6] : "") + "\"}";
        first = false;
    }
    result += "\n]";
    return result;
}

UserInfo ExchangeManager::getUserByKey(string key) {
    stringstream ss;
    engine.execute("SELECT user.user_pk FROM user WHERE user.key = '" + key + "'", ss);
    string id = trim(ss.str());
    return {id, key};
}

void ExchangeManager::updateBalance(string userId, string lotId, double delta) {
    cout << "[UPDATE] Изменение баланса пользователя " << userId << " для лота " << lotId << " на сумму " << delta << endl;
    
    stringstream ss;
    engine.execute("SELECT user_lot.user_lot_pk, user_lot.quantity FROM user_lot WHERE user_lot.user_id = '" + userId + "' AND user_lot.lot_id = '" + lotId + "'", ss);
    
    string line;
    if (getline(ss, line)) {
        auto parts = split(line, ',');
        if (parts.size() >= 2) {
            double current = stod(parts[1]);
            double nextVal = current + delta;
            cout << "[UPDATE] Старый баланс: " << current << ", Новый баланс: " << nextVal << endl;
            
            executeQuery("DELETE FROM user_lot WHERE user_lot.user_lot_pk = " + parts[0]);
            executeQuery("INSERT INTO user_lot VALUES ('" + userId + "', '" + lotId + "', '" + to_string(nextVal) + "')");
        }
    } else {
        cout << "[UPDATE] Ошибка: Запись в user_lot не найдена для пользователя " << userId << " и лота " << lotId << endl;
    }
}

string ExchangeManager::createOrder(string key, int pairId, double quantity, double price, string type) {
    UserInfo user = getUserByKey(key);
    if (user.id.empty() || user.id == "NULL") return "{\"Ошибка\": \"Неверный ключ\"}";

    stringstream ss_p;
    engine.execute("SELECT pair.first_lot_id, pair.second_lot_id FROM pair WHERE pair.pair_pk = " + to_string(pairId), ss_p);
    string p_line;
    if (!getline(ss_p, p_line)) return "{\"Ошибка\": \"Пара не найдена\"}";
    auto p_parts = split(p_line, ',');
    
    string lotToLock = (type == "buy") ? p_parts[1] : p_parts[0];
    double amountToLock = (type == "buy") ? (quantity * price) : quantity;

    cout << "[ORDER] Создание ордера " << type << ". Блокировка актива " << lotToLock << " в размере " << amountToLock << endl;
    updateBalance(user.id, lotToLock, -amountToLock);

    string matchType = (type == "buy") ? "sell" : "buy";
    stringstream ss_m;
    engine.execute("SELECT * FROM order WHERE order.pair_id = '" + to_string(pairId) + "' AND order.type = '" + matchType + "' AND order.closed = ''", ss_m);
    
    double remaining = quantity;
    string line;
    while (getline(ss_m, line) && remaining > 0) {
        auto m = split(line, ',');
        double m_price = stod(m[4]);
        double m_qty = stod(m[3]);
        
        if ((type == "buy" && m_price <= price) || (type == "sell" && m_price >= price)) {
            double tradeQty = min(remaining, m_qty);
            cout << "[MATCH] Сделка! Объем: " << tradeQty << ", Цена: " << m_price << endl;

            if (type == "buy") {
                updateBalance(user.id, p_parts[0], tradeQty);
                updateBalance(m[1], p_parts[1], tradeQty * m_price);
            } else {
                updateBalance(user.id, p_parts[1], tradeQty * m_price);
                updateBalance(m[1], p_parts[0], tradeQty);
            }
            
            executeQuery("DELETE FROM order WHERE order.order_pk = " + m[0]);
            if (m_qty > tradeQty) {
                executeQuery("INSERT INTO order VALUES ('" + m[1] + "', '" + m[2] + "', '" + to_string(m_qty - tradeQty) + "', '" + m[4] + "', '" + m[5] + "', '')");
            } else {
                executeQuery("INSERT INTO order VALUES ('" + m[1] + "', '" + m[2] + "', '" + to_string(m_qty) + "', '" + m[4] + "', '" + m[5] + "', '1730738627')");
            }
            remaining -= tradeQty;
        }
    }

    if (remaining > 0) {
        executeQuery("INSERT INTO order VALUES ('" + user.id + "', '" + to_string(pairId) + "', '" + to_string(remaining) + "', '" + to_string(price) + "', '" + type + "', '')");
    }
    return "{\"Статус\": \"Успешно\"}";
}

string ExchangeManager::deleteOrder(string key, int orderId) {
    UserInfo user = getUserByKey(key);
    stringstream ss;
    engine.execute("SELECT * FROM order WHERE order.order_pk = " + to_string(orderId), ss);
    string line;
    if (getline(ss, line)) {
        auto o = split(line, ',');
        if (o[1] != user.id) return "{\"Ошибка\": \"Нет доступа\"}";
        
        stringstream ss_p;
        engine.execute("SELECT pair.first_lot_id, pair.second_lot_id FROM pair WHERE pair.pair_pk = " + o[2], ss_p);
        string p_line; 
        getline(ss_p, p_line); 
        auto p = split(p_line, ',');
        
        double refund = (o[5] == "buy") ? (stod(o[3]) * stod(o[4])) : stod(o[3]);
        string lotToRefund = (o[5] == "buy" ? p[1] : p[0]);
        
        cout << "[DELETE] Отмена ордера. Возврат " << refund << " лота " << lotToRefund << endl;
        updateBalance(user.id, lotToRefund, refund);
        
        executeQuery("DELETE FROM order WHERE order.order_pk = " + to_string(orderId));
        return "{\"Статус\": \"Удалено\"}";
    }
    return "{\"Ошибка\": \"Ордер не найден\"}";
}
