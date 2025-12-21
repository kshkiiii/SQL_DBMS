#include "schema.h"
#include <fstream>
#include <filesystem>
#include <regex>

using namespace std;
namespace fs = filesystem;

DatabaseSchema SchemaManager::loadSchema(const string& filepath) {
    ifstream f(filepath);
    if (!f.is_open()) throw runtime_error("schema.json не найден");
    
    stringstream buffer;
    buffer << f.rdbuf();
    string content = buffer.str();

    DatabaseSchema schema;
    smatch match;

    if (regex_search(content, match, regex(R"json("name"\s*:\s*"([^"]+)")json"))) {
        schema.name = match[1].str();
    }
    
    if (regex_search(content, match, regex(R"json("tuples_limit"\s*:\s*(\d+))json"))) {
        schema.tuples_limit = stoi(match[1].str());
    }

    regex tableRegex(R"json(\"([a-zA-Z0-9_]+)\"\s*:\s*\[([^\]]+)\])json"); 
    auto it = sregex_iterator(content.begin(), content.end(), tableRegex);
    
    for (; it != sregex_iterator(); ++it) {
        string tName = (*it)[1].str();
        TableSchema tb;
        tb.name = tName;
        tb.columns.push_back(tName + "_pk"); 

        string colsStr = (*it)[2].str();
        regex colContentRegex(R"json(\"([^\"]+)\")json"); 
        auto cit = sregex_iterator(colsStr.begin(), colsStr.end(), colContentRegex);
        for (; cit != sregex_iterator(); ++cit) {
             tb.columns.push_back((*cit)[1].str());
        }
        schema.tables[tName] = tb;
    }

    return schema;
}

void SchemaManager::initDirectories(const DatabaseSchema& schema) {
    if (!fs::exists(schema.name)) fs::create_directory(schema.name);
    for (const auto& [name, table] : schema.tables) {
        string path = schema.name + "/" + name;
        if (!fs::exists(path)) {
            fs::create_directories(path);
            ofstream seq(path + "/" + name + "_pk_sequence");
            seq << 1; 
        }
    }
}
