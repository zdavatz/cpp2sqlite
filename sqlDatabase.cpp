//
//  sqlDatabase.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 16 Jan 2019
//

#include <iostream>
#include <sstream>
#include <sqlite3.h>
#include <libgen.h>     // for basename()

#include "sqlDatabase.hpp"

// See SqlDatabase.java:65
#define FI_DB_VERSION   "140"

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
            std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", rc" << rc << std::endl;
    }
}

void destroyStatement(sqlite3_stmt * statement)
{
    // Destroy the object
    int rc = sqlite3_finalize(statement);
    if ((rc != SQLITE_OK) && (rc != SQLITE_DONE))
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", error " << rc << std::endl;
}

void runStatement(const std::string &tableName,
                  sqlite3_stmt * statement)
{
    // Run the SQL
    int rc = sqlite3_step(statement);
    if ((rc != SQLITE_OK) && (rc != SQLITE_DONE))
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__
                  << ", statement: " << sqlite3_expanded_sql(statement)
                  << ", error: " << rc << std::endl;
    

    rc = sqlite3_reset(statement);
}

void bindText(const std::string &tableName,
              sqlite3_stmt * statement,
              int pos,
              const std::string &text)
{
    //std::cout << basename((char *)__FILE__) << ":" << __LINE__
    //          << " pos:" << pos << " text:" << text << std::endl;

    int rc = sqlite3_bind_text(statement, pos, text.c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK)
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", error " << rc << std::endl;
}

void prepareStatement(const std::string &tableName,
                      sqlite3_stmt ** statement,
                      const std::string &placeholders)
{
    std::ostringstream sqlStream;
    sqlStream << "INSERT INTO " << tableName
              << " VALUES (" << placeholders << ");";
    //std::cout << basename((char *)__FILE__) << ":" << __LINE__ << " " << sqlStream.str() << std::endl;
    
    int rc = sqlite3_prepare_v2(db, sqlStream.str().c_str(), -1, statement, NULL);
    if (rc != SQLITE_OK)
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", error " << rc << std::endl;
}

void insertInto(const std::string &tableName,
                const std::string &keys,
                const std::string &values)
{
    std::ostringstream sqlStream;
    sqlStream << "INSERT INTO " << tableName
              << " (" << keys << ") "
              << "VALUES (" << values << ");";
    //std::cout << basename((char *)__FILE__) << ":" << __LINE__ << " " << sqlStream.str() << std::endl;
    int rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__
                  << ", sqlite3_exec error " << rc
                  << ", " << sqlStream.str()
                  << std::endl;
    }
}

void createTable(const std::string &tableName, const std::string &keys)
{
    std::ostringstream sqlStream;
    int rc;

    sqlStream << "PRAGMA user_version=" << FI_DB_VERSION << ";";
    rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", rc" << rc << std::endl;
    
    sqlStream << "DROP TABLE IF EXISTS " << tableName << ";";
    rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", rc" << rc << std::endl;
    
    // See SqlDatabase.java 207
    sqlStream.str("");
    sqlStream << "CREATE TABLE " << tableName << "(" << keys.c_str() << ");";
    rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", rc" << rc << std::endl;
}

sqlite3 * createDB(const std::string &filename)
{
    int rc = sqlite3_open(filename.c_str(), &db);
    if (rc != SQLITE_OK)
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", rc" << rc << std::endl;

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
