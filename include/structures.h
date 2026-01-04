#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <mutex>

using namespace std;

inline string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

inline vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    if (!s.empty() && s.back() == delimiter) {
        tokens.push_back("");
    }
    return tokens;
}

struct TableSchema {
    string name;
    vector<string> columns; 
};

struct DatabaseSchema {
    string name;
    int tuples_limit;
    map<string, TableSchema> tables;
};

using Row = map<string, string>;
