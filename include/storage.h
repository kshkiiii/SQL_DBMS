#pragma once
#include "structures.h"
#include <fstream>
#include <memory>
#include <filesystem>
#include <mutex>

using namespace std;

class Cursor {
public:
    virtual bool next() = 0;
    virtual Row getRow() = 0;
    virtual void reset() = 0; 
    virtual ~Cursor() = default;
};

class TableLock {
    string lockPath;
    bool acquired;
public:
    TableLock(const string& path);
    ~TableLock();
    bool isAcquired() const;
};

class StorageManager {
    DatabaseSchema schema;
    mutex mtx;
    int getNextId(const string& tableName);

public:
    StorageManager(DatabaseSchema s) : schema(s) {}
    void insert(const string& tableName, const vector<string>& values);
    void deleteRows(const string& tableName, const string& whereCol, const string& whereVal);
    unique_ptr<Cursor> getCursor(const string& tableName);
};
