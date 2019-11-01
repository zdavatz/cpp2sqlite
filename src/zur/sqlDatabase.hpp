//
//  sqlDatabase.hpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 30 Oct 2019
//

#ifndef sqlDatabase_hpp
#define sqlDatabase_hpp

#include <sqlite3.h>
#include <vector>

namespace DB
{

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
};


}

#endif /* sqlDatabase_hpp */
