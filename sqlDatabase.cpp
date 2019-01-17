//
//  sqlDatabase.cpp
//  cpp2sqlite
//
//  Created by Alex Bettarini on 16 Jan 2019
//

#include <iostream>
#include <sstream>
#include <sqlite3.h>
#include <libgen.h>     // for basename()

#include "sqlDatabase.hpp"

namespace AIPS
{
    static sqlite3 *db;

void createIndex(const std::string &tableName,
                 const std::string &prefix,
                 const std::vector<std::string> &keys)
{

    for (std::string k : keys) {
        std::string indexName = prefix + k;

        std::ostringstream sqlStream;
        sqlStream << "CREATE INDEX " << indexName << " ON " << tableName << "(" << k << ");";
        //std::cout << "Line: " << __LINE__ << " " << sqlStream.str() << std::endl;
        int rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, NULL);
        if (rc != SQLITE_OK)
            std::cout << "Line: " << __LINE__ << ", rc" << rc << std::endl;
    }
}

void insertInto(const std::string &tableName, const std::string &keys, const std::string &values)
{
#if 1
    sqlite3_stmt * statement;
    std::ostringstream sqlStream;
    sqlStream << "INSERT INTO " << tableName
              << " (" << keys << ") "
              << "VALUES (?, ?);";
    int rc = sqlite3_prepare(db, sqlStream.str().c_str(), -1, &statement, NULL);
    rc = sqlite3_bind_text(statement, 1, "", -1, SQLITE_STATIC);
    rc = sqlite3_step(statement);
    rc = sqlite3_finalize(statement);
#else
    std::ostringstream sqlStream;
    sqlStream << "INSERT INTO " << tableName
              << " (" << keys << ") "
              << "VALUES (" << values << ");";
    //std::cout << "Line: " << __LINE__ << " " << sqlStream.str() << std::endl;
    int rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        std::cout << basename((char *)__FILE__) << ":" << __LINE__
                  << ", sqlite3_exec error " << rc
                  << ", " << sqlStream.str()
                  << std::endl;
    }
#endif
}
    
void createTable(const std::string &tableName, const std::string &keys)
{
    std::ostringstream sqlStream;
    int rc;

    sqlStream << "DROP TABLE IF EXISTS " << tableName << ";";
    rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        std::cout << "Line: " << __LINE__ << ", rc" << rc << std::endl;
    
    // See SqlDatabase.java 207
    sqlStream.str("");
    sqlStream << "CREATE TABLE " << tableName << "(" << keys.c_str() << ");";
    rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        std::cout << "Line: " << __LINE__ << ", rc" << rc << std::endl;
}

sqlite3 * createDB(const std::string &filename)
{
    //const char *dbFilename = "amiko_db_full_idx_de.db";
    
    int rc = sqlite3_open(filename.c_str(), &db);
    if (rc != SQLITE_OK)
        std::cout << "Line: " << __LINE__ << ", rc" << rc << std::endl;

    createTable("amikodb", "_id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, auth TEXT, atc TEXT, substances TEXT, regnrs TEXT, atc_class TEXT, tindex_str TEXT, application_str TEXT, indications_str TEXT, customer_id INTEGER, pack_info_str TEXT, add_info_str TEXT, ids_str TEXT, titles_str TEXT, content TEXT, style_str TEXT, packages TEXT");
    createIndex("amikodb", "idx_", {"title", "auth", "atc", "substances", "regnrs", "atc_class"});

    createTable("productdb", "_id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, author TEXT, eancodes TEXT, pack_info_str TEXT, packages TEXT");
    createIndex("productdb", "idx_prod_", {"title", "author", "eancodes"});
    
    createTable("android_metadata", "locale TEXT default 'en_US'");
    insertInto("android_metadata", "locale", "'en_US'");

    //createTable("sqlite_sequence", "");  // created automatically
    return db;
}

}
