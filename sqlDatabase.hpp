//
//  sqlDatabase.hpp
//  cpp2sqlite
//
//  Created by Alex Bettarini on 16 Jan 2019
//

#ifndef sqlDatabase_hpp
#define sqlDatabase_hpp

#include <vector>

namespace AIPS
{
    sqlite3 * createDB(const std::string &filename);

    void createTable(const std::string &tableName, const std::string &keys);
    void createIndex(const std::string &tableName, const std::string &prefix, const std::vector<std::string> &keys);
    
    void insertInto(const std::string &tableName, const std::string &keys, const std::string &values);
}

#endif /* sqlDatabase_hpp */
