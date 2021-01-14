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
#include <libgen.h>     // for basename()

#include <boost/algorithm/string.hpp>
#include "drugshortage.hpp"
#include "report.hpp"

namespace DRUGSHORTAGE
{
    std::map<int64_t, nlohmann::json> drugshortageJsonMap;
    std::map<int64_t, int64_t> colourCodeCounts;
    
    void parseJSON (const std::string &filename) {
        try {
            std::clog << std::endl << "Reading " << filename << std::endl;

            std::ifstream jsonInputStream(filename);
            nlohmann::json drugshortageJson;
            jsonInputStream >> drugshortageJson;
            for (nlohmann::json::iterator it = drugshortageJson.begin(); it != drugshortageJson.end(); ++it) {
                auto entry = it.value();
                int64_t thisGtin = entry["gtin"].get<int64_t>();
                drugshortageJsonMap[thisGtin] = entry;

                try {
                    int colourCode = entry["colorCode"]["#"].get<int64_t>();
                    if (colourCodeCounts.find(colourCode) == colourCodeCounts.end()) {
                        colourCodeCounts[colourCode] = 1;
                    } else {
                        colourCodeCounts[colourCode]++;
                    }
                }catch(...){}
            }
        }
        catch (std::exception &e) {
            std::cerr
            << basename((char *)__FILE__) << ":" << __LINE__
            << " Error " << e.what()
            << std::endl;
        }
        printFileStats(filename);
    }
    
    nlohmann::json getEntryByGtin(int64_t gtin) {
        if (drugshortageJsonMap.find(gtin) != drugshortageJsonMap.end()) {
            return drugshortageJsonMap[gtin];
        }
        return nlohmann::json::object();
    }

    static void printFileStats(const std::string &filename)
    {
        REP::html_h2("Drugshortage");

        REP::html_p(filename);
        
        REP::html_start_ul();
        REP::html_li("GTINs : " + std::to_string(drugshortageJsonMap.size()));
        for (int i = 1; i <= 5; i++) {
            int64_t count = colourCodeCounts.find(i) != colourCodeCounts.end() ? colourCodeCounts[i] : 0;
            REP::html_li(stringForColourCode(i) + "(" + std::to_string(i) + "): " + std::to_string(count));
        }
        REP::html_end_ul();
    }

    static std::string stringForColourCode(int64_t number) {
        switch (number) {
            case 1:
                return "Total grün";
            case 2:
                return "Total hellgrün";
            case 3:
                return "Total orange";
            case 4:
                return "Total rot";
            case 5:
                return "Total Gelb.";
            default:
                return "";
        }
    }
}