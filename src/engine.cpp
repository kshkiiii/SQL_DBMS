#include "engine.h"
#include <regex>

using namespace std;

bool QueryEngine::checkAndCondition(const Row& row, const string& andClause) {
    regex and_re("\\s+AND\\s+", regex::icase);
    sregex_token_iterator it(andClause.begin(), andClause.end(), and_re, -1), end;
    for (; it != end; ++it) {
        auto parts = split(*it, '=');
        if (parts.size() != 2) continue;
        string l = parts[0], r = parts[1];
        if (row.count(l)) l = row.at(l);
        if (row.count(r)) r = row.at(r);
        if (l.size()>=2 && l.front()=='\'') l = l.substr(1, l.size()-2);
        if (r.size()>=2 && r.front()=='\'') r = r.substr(1, r.size()-2);
        if (trim(l) != trim(r)) return false;
    }
    return true;
}

bool QueryEngine::checkCondition(const Row& row, const string& whereClause) {
    if (trim(whereClause).empty()) return true;
    regex or_re("\\s+OR\\s+", regex::icase);
    sregex_token_iterator it(whereClause.begin(), whereClause.end(), or_re, -1), end;
    for (; it != end; ++it) if (checkAndCondition(row, *it)) return true;
    return false;
}

void QueryEngine::execute(string query, ostream& out) {
    query = trim(query);
    if (query.empty()) return;
    if (query.back() == ';') query.pop_back();

    smatch m;
    if (regex_match(query, m, regex(R"(INSERT\s+INTO\s+([^\s]+)\s+VALUES\s*\((.*)\))", regex::icase))) {
        string table = m[1];
        auto vals = split(m[2], ',');
        for(auto& v : vals) {
            v = trim(v);
            if(v.size()>=2 && v.front()=='\'') v = v.substr(1, v.size()-2);
        }
        try {
            storage.insert(table, vals);
            out << "Успешно" << endl;
        } catch (exception& e) {
            out << "Ошибка: " << e.what() << endl;
        }
        return;
    }

    if (regex_match(query, m, regex(R"(DELETE\s+FROM\s+([^\s]+)\s+WHERE\s+([^\s=]+)\s*=\s*(.*))", regex::icase))) {
        string table = m[1];
        string col = m[2];
        string val = trim(m[3]);
        if(val.size() >= 2 && val.front() == '\'' && val.back() == '\'') val = val.substr(1, val.size()-2);
        try {
            storage.deleteRows(table, col, val);
            out << "Успешно" << endl;
        } catch (exception& e) {
            out << "Ошибка: " << e.what() << endl;
        }
        return;
    }

    if (regex_match(query, m, regex(R"(SELECT\s+(.*)\s+FROM\s+([^\s,]+)(?:\s*,\s*([^\s,]+))?(?:\s+WHERE\s+(.*))?)", regex::icase))) {
        string colsPart = m[1], t1 = m[2], t2 = m[3], wherePart = m[4];
        vector<string> tables = {t1};
        if (!t2.empty()) tables.push_back(t2);

        vector<unique_ptr<Cursor>> cursors;
        try {
            for(auto& t : tables) cursors.push_back(storage.getCursor(t));
        } catch (exception& e) {
            out << "Ошибка: " << e.what() << endl;
            return;
        }

        auto reqCols = split(colsPart, ',');
        bool all = (reqCols.size()==1 && reqCols[0]=="*");

        if (cursors.size() == 1) {
            while(cursors[0]->next()) {
                Row r = cursors[0]->getRow();
                if (checkCondition(r, wherePart)) {
                    if (all) {
                        auto& schemaCols = schema.tables[t1].columns;
                        for(size_t i=0; i<schemaCols.size(); ++i) 
                            out << r[t1+"."+schemaCols[i]] << (i == schemaCols.size()-1 ? "" : ", ");
                    } else {
                        for(size_t i=0; i<reqCols.size(); ++i)
                            out << (r.count(reqCols[i]) ? r[reqCols[i]] : "NULL") << (i == reqCols.size()-1 ? "" : ", ");
                    }
                    out << endl;
                }
            }
        } else {
            while(cursors[0]->next()) {
                cursors[1]->reset();
                while(cursors[1]->next()) {
                    Row combined = cursors[0]->getRow(), r2 = cursors[1]->getRow();
                    combined.insert(r2.begin(), r2.end());
                    if (checkCondition(combined, wherePart)) {
                        if (all) {
                            vector<string> allCols;
                            for(auto& c : schema.tables[t1].columns) allCols.push_back(t1+"."+c);
                            for(auto& c : schema.tables[t2].columns) allCols.push_back(t2+"."+c);
                            for(size_t i=0; i<allCols.size(); ++i)
                                out << combined[allCols[i]] << (i == allCols.size()-1 ? "" : ", ");
                        } else {
                            for(size_t i=0; i<reqCols.size(); ++i)
                                out << (combined.count(reqCols[i]) ? combined[reqCols[i]] : "NULL") << (i == reqCols.size()-1 ? "" : ", ");
                        }
                        out << endl;
                    }
                }
            }
        }
        return;
    }
    out << "Неизвестная команда" << endl;
}
