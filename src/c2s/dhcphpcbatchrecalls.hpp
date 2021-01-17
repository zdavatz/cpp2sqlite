//
//  dhcphpcbatchrecalls.hpp
//  dhcphpcbatchrecalls
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 9 Jan 2021
//
// https://github.com/zdavatz/cpp2sqlite/issues/140

#ifndef dhcphpcbatchrecalls_hpp
#define dhcphpcbatchrecalls_hpp

#include <nlohmann/json.hpp>

namespace DHCPHPCBATCHRECALLS
{
    class Recall {
    public:
        std::string title;
        std::string date;
        std::string preparation;
        std::string regnrs;
        std::string substance;
        std::string licensee;
        std::string withdrawalOfTheaBatch;
        std::string description;
        std::string pdfLink;
    };
    void parseJSON(const std::string &filename);
    Recall jsonToRecall(nlohmann::json entry);
    Recall getRecallByRegnrs(std::string regnrs);
    static void printFileStats(const std::string &filename);
}

#endif /* dhcphpcbatchrecalls_hpp */
