//
//  dhpchpc.hpp
//  dhpchpc
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 30 Jan 2021
//
// https://github.com/zdavatz/cpp2sqlite/issues/140

#ifndef dhpchpc_hpp
#define dhpchpc_hpp

#include <nlohmann/json.hpp>

namespace DHPCHPC
{
    class News {
    public:
        std::string title;
        std::string date;
        std::string regnrs;
        std::string description;
        std::string pdfLink;
        std::map<std::string, std::string> extras;
    };
    void parseJSON(const std::string &filename);
    News jsonToNews(nlohmann::json entry);
    std::vector<News> getNewsByRegnrs(std::string regnrs);
    static void printFileStats(const std::string &filename);
}

#endif /* dhpchpc_hpp */
