//
//  sqlDatabase.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 30 Oct 2019
//

#include <iostream>
#include <sstream>
#include <sqlite3.h>
#include <libgen.h>     // for basename()

#include "sqlDatabase.hpp"

// See DispoParse.java:65
#define FI_DB_VERSION   "140"

namespace DB
{

void Sql::createIndex(const std::string_view &tableName,
                      const std::string &prefix,
                      const std::vector<std::string> &keys)
{
    char *errmsg;
    for (std::string k : keys) {
        std::string indexName = prefix + k;

        std::ostringstream sqlStream;
        sqlStream << "CREATE INDEX " << indexName << " ON " << tableName << "(" << k << ");";
        //std::cout << "Line: " << __LINE__ << " " << sqlStream.str() << std::endl;
        int rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, &errmsg);
        if (rc != SQLITE_OK)
            std::cerr
            << basename((char *)__FILE__) << ":" << __LINE__
            << ", error " << rc
            << ", " << errmsg
            << std::endl;
    }
}

void Sql::destroyStatement()
{
    // Destroy the object
    int rc = sqlite3_finalize(statement);
    if ((rc != SQLITE_OK) && (rc != SQLITE_DONE))
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << std::endl;
}

void Sql::runStatement(const std::string_view &tableName)
{
    int rc = sqlite3_step(statement);
    if ((rc != SQLITE_OK) && (rc != SQLITE_DONE)) {
        char *exp = sqlite3_expanded_sql(statement);
        std::string expString(exp);
        sqlite3_free(exp);

        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << ", statement: " << expString.substr(0,200)
        << std::endl;
    }

    rc = sqlite3_reset(statement);
}

void Sql::bindBool(int pos, bool flag)
{
    int val = flag ? 1 : 0;
    bindInt(pos, val);    
}

void Sql::bindInt(int pos, int val)
{
    int rc = sqlite3_bind_int(statement, pos, val);
    if (rc != SQLITE_OK)
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << std::endl;
}

void Sql::bindText(int pos, const std::string &text)
{
    //std::cout << basename((char *)__FILE__) << ":" << __LINE__
    //          << " pos:" << pos << " text:" << text << std::endl;

    int rc = sqlite3_bind_text(statement, pos, text.c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK)
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << std::endl;
}

void Sql::prepareStatement(const std::string_view &tableName,
                           const std::string &placeholders)
{
    std::ostringstream sqlStream;
    sqlStream << "INSERT INTO " << tableName
              << " VALUES (" << placeholders << ");";
    //std::cout << basename((char *)__FILE__) << ":" << __LINE__ << " " << sqlStream.str() << std::endl;
    
    int rc = sqlite3_prepare_v2(db, sqlStream.str().c_str(), -1, &statement, NULL);
    if (rc != SQLITE_OK)
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << std::endl;
}

void Sql::insertInto(const std::string_view &tableName,
                     const std::string &keys,
                     const std::string &values)
{
    std::ostringstream sqlStream;
    int rc;
    char *errmsg;

    sqlStream << "INSERT INTO " << tableName
              << " (" << keys << ") "
              << "VALUES (" << values << ");";
    //std::cout << basename((char *)__FILE__) << ":" << __LINE__ << " " << sqlStream.str() << std::endl;
    rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << ", " << errmsg
        << ", " << sqlStream.str().substr(0,100)
        << std::endl;
    }
}

void Sql::createTable(const std::string_view &tableName,
                      const std::string &keys)
{
    std::ostringstream sqlStream;
    int rc;
    char *errmsg;

    sqlStream << "PRAGMA user_version=" << FI_DB_VERSION << ";";
    rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, &errmsg);
    if (rc != SQLITE_OK)
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << ", " << errmsg
        << std::endl;
    
    sqlStream << "DROP TABLE IF EXISTS " << tableName << ";";
    rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, &errmsg);
    if (rc != SQLITE_OK)
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << ", " << errmsg
        << std::endl;
    
    // See SqlDatabase.java 207
    sqlStream.str("");
    sqlStream << "CREATE TABLE " << tableName << "(" << keys.c_str() << ");";
    rc = sqlite3_exec(db, sqlStream.str().c_str(), NULL, NULL, &errmsg);
    if (rc != SQLITE_OK)
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << ", " << errmsg
        << std::endl;
}

void Sql::openDB(const std::string &filename)
{
    int rc = sqlite3_open(filename.c_str(), &db);
    if (rc != SQLITE_OK)
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << std::endl;
}

void Sql::closeDB()
{
    int rc = sqlite3_close(db);
    if (rc != SQLITE_OK)
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", rc:" << rc
        << std::endl;
}

}
