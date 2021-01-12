//
//  drugshortage.cpp
//  drugshortage
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 9 Jan 2021
//
// https://github.com/zdavatz/cpp2sqlite/issues/140


#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <map>

#include <boost/algorithm/string.hpp>
#include "drugshortage.hpp"

namespace DRUGSHORTAGE
{
    std::map<int64_t, nlohmann::json> drugshortageJsonMap;
    
    void parseJSON (const std::string &filename) {
        std::ifstream jsonInputStream(filename);
        nlohmann::json drugshortageJson;
        jsonInputStream >> drugshortageJson;
        for (nlohmann::json::iterator it = drugshortageJson.begin(); it != drugshortageJson.end(); ++it) {
            auto entry = it.value();
            int64_t thisGtin = entry["gtin"].get<int64_t>();
            drugshortageJsonMap[thisGtin] = entry;
        }
    }
    
    nlohmann::json getEntryByGtin(int64_t gtin) {
        if (drugshortageJsonMap.find(gtin) != drugshortageJsonMap.end()) {
            return drugshortageJsonMap[gtin];
        }
        return nlohmann::json::object();
    }
}