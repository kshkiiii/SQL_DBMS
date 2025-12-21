#pragma once
#include "storage.h"

using namespace std;

class QueryEngine {
    DatabaseSchema schema;
    StorageManager storage;

    bool checkCondition(const Row& row, const string& whereClause);
    bool checkAndCondition(const Row& row, const string& andClause);

public:
    QueryEngine(DatabaseSchema s) : schema(s), storage(s) {}
    void execute(string query, ostream& out);
};