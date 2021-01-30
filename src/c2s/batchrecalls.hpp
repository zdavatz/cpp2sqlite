//
//  batchrecalls.hpp
//  batchrecalls
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 9 Jan 2021
//
// https://github.com/zdavatz/cpp2sqlite/issues/140

#ifndef batchrecalls_hpp
#define batchrecalls_hpp

#include <nlohmann/json.hpp>

namespace BATCHRECALLS
{
    class Recall {
    public:
        std::string title;
        std::string date;
        std::string regnrs;
        std::vector<std::string> regnrsParsed;
        std::string description;
        std::string pdfLink;
        std::map<std::string, std::string> extras;
    };
    void parseJSON(const std::string &filename);
    Recall jsonToRecall(nlohmann::json entry);
    std::vector<Recall> getRecallsByRegnrs(std::string regnrs);
    static void printFileStats(const std::string &filename);
}

#endif /* batchrecalls_hpp */
