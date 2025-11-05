//
//  sqlDatabase.hpp
//  cpp2sqlite, zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 16 Jan 2019, 30 Oct 2019
//

#ifndef sqlDatabase_hpp
#define sqlDatabase_hpp

#include <sqlite3.h>
#include <vector>

namespace DB
{

struct RowToInsert {
    // _id INTEGER PRIMARY KEY AUTOINCREMENT,
    std::string title;
    std::string auth;
    std::string atc;
    std::string substances;
    std::string regnrs;
    std::string atc_class;
    std::string tindex_str;
    std::string application_str;
    std::string indications_str;
    int customer_id = 0;
    std::string pack_info_str;
    std::string add_info_str;
    std::string ids_str;
    std::string titles_str;
    std::string content;
    std::string style_str;
    std::string packages;
};

struct Sql
{
private:
    sqlite3 *db;
    sqlite3_stmt *statement;

public:
    void openDB(const std::string &filename);
    void closeDB();

    void createTable(const std::string_view &tableName,
                     const std::string &keys);

    void createIndex(const std::string_view &tableName,
                     const std::string &prefix,
                     const std::vector<std::string> &keys);

    void prepareStatement(const std::string_view &tableName,
                          const std::string &placeholders);

    void destroyStatement();

    void bindBool(int pos, bool flag);
    void bindInt(int pos, int val);
    void bindText(int pos, const std::string &text);

    void runStatement(const std::string_view &tableName);

    void insertInto(const std::string_view &tableName,
                    const std::string &keys,
                    const std::string &values);
    void insertRow(const std::string_view &tableName, DB::RowToInsert row);
};

}

#endif /* sqlDatabase_hpp */
