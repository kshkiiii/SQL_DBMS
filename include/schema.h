#pragma once
#include "structures.h"

using namespace std;

class SchemaManager {
public:
    static DatabaseSchema loadSchema(const string& filepath);
    static void initDirectories(const DatabaseSchema& schema);
};