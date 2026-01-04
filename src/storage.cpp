#include "storage.h"
#include <thread>
#include <chrono>

using namespace std;
namespace fs = filesystem;

const int LOCK_DELAY = 0;

TableLock::TableLock(const string& path) : lockPath(path), acquired(false) {
    if (fs::create_directory(lockPath)) acquired = true;
}

TableLock::~TableLock() {
    if (acquired) fs::remove(lockPath);
}

bool TableLock::isAcquired() const {
    return acquired;
}

class TableCursor : public Cursor {
    StorageManager& manager;
    DatabaseSchema& schema;
    string tableName;
    vector<string> columns;
    int currentFileIndex = 1;
    ifstream currentStream;
    Row currentRow;
    bool finished = false;

    bool openNextFile() {
        if (currentStream.is_open()) currentStream.close();
        string path = schema.name + "/" + tableName + "/" + to_string(currentFileIndex++) + ".csv";
        if (!fs::exists(path)) return false;
        currentStream.open(path);
        return true;
    }

public:
    TableCursor(StorageManager& m, DatabaseSchema& s, string tName) : manager(m), schema(s), tableName(tName) {
        columns = schema.tables[tName].columns;
        reset();
    }

    void reset() override {
        currentFileIndex = 1;
        finished = false;
        if (currentStream.is_open()) currentStream.close();
        openNextFile();
    }

    bool next() override {
        if (finished) return false;
        string line;
        if (!getline(currentStream, line)) {
            if (openNextFile()) return next();
            finished = true;
            return false;
        }
        if (line.empty()) return next();
        auto values = split(line, ',');
        currentRow.clear();
        for (size_t i = 0; i < values.size() && i < columns.size(); ++i) {
            currentRow[tableName + "." + columns[i]] = values[i];
        }
        return true;
    }

    Row getRow() override { return currentRow; }
};

unique_ptr<Cursor> StorageManager::getCursor(const string& tableName) {
    return make_unique<TableCursor>(*this, schema, tableName);
}

int StorageManager::getNextId(const string& tableName) {
    lock_guard<mutex> lock(mtx);
    string path = schema.name + "/" + tableName + "/" + tableName + "_pk_sequence";
    int id = 1;
    if (fs::exists(path)) {
        ifstream seqIn(path);
        seqIn >> id;
    }
    ofstream seqOut(path, ios::trunc);
    seqOut << (id + 1);
    return id;
}

void StorageManager::insert(const string& tableName, const vector<string>& values) {
    TableLock lock(schema.name + "/" + tableName + "/" + tableName + "_lock");
    if (!lock.isAcquired()) throw runtime_error("Таблица заблокирована");

    int fileIdx = 1;
    string path;
    while(true) {
        path = schema.name + "/" + tableName + "/" + to_string(fileIdx) + ".csv";
        if (!fs::exists(path)) break; 
        ifstream in(path);
        int lines = 0;
        string line;
        while(getline(in, line)) lines++;
        if (lines < schema.tuples_limit) break; 
        fileIdx++;
    }

    this_thread::sleep_for(chrono::seconds(LOCK_DELAY));

    ofstream out(path, ios::app);
    out << getNextId(tableName);
    for (const auto& v : values) out << "," << v;
    out << "\n";
}

void StorageManager::deleteRows(const string& tableName, const string& whereCol, const string& whereVal) {
    TableLock lock(schema.name + "/" + tableName + "/" + tableName + "_lock");
    if (!lock.isAcquired()) throw runtime_error("Таблица заблокирована");

    int colIdx = -1;
    string cleanCol = whereCol;
    if(cleanCol.find('.') != string::npos) cleanCol = cleanCol.substr(cleanCol.find('.') + 1);
    auto& cols = schema.tables[tableName].columns;
    for(size_t i=0; i<cols.size(); ++i) if(cols[i] == cleanCol) { colIdx = i; break; }
    if (colIdx == -1) throw runtime_error("Столбец не найден");

    this_thread::sleep_for(chrono::seconds(LOCK_DELAY));

    int fileIdx = 1;
    while (true) {
        string path = schema.name + "/" + tableName + "/" + to_string(fileIdx++) + ".csv";
        if (!fs::exists(path)) break;
        ifstream in(path);
        string tempPath = path + ".tmp", line;
        ofstream out(tempPath);
        while (getline(in, line)) {
            auto rowVals = split(line, ',');
            if (!(colIdx < rowVals.size() && trim(rowVals[colIdx]) == trim(whereVal))) out << line << "\n";
        }
        in.close(); out.close();
        fs::remove(path); fs::rename(tempPath, path);
    }
}
