//
//  drugshortage.hpp
//  drugshortage
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 9 Jan 2021
//
// https://github.com/zdavatz/cpp2sqlite/issues/140

#ifndef drugshortage_hpp
#define drugshortage_hpp

#include <nlohmann/json.hpp>

namespace DRUGSHORTAGE
{
    void parseJSON(const std::string &filename);
    
    nlohmann::json getEntryByGtin(int64_t gtin);
}

#endif /* drugshortage_hpp */
