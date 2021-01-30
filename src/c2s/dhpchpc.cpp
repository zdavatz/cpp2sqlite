//
//  dhpchpc.cpp
//  dhpchpc
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 30 Jan 2021
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
#include "dhpchpc.hpp"
#include "report.hpp"

namespace DHPCHPC
{
    std::map<std::string, std::vector<News>> regnrsToNews;
    int64_t withoutRegnrsCount = 0;
    void parseJSON(const std::string &filename) {
        try {
            std::clog << std::endl << "Reading " << filename << std::endl;

            std::ifstream jsonInputStream(filename);
            nlohmann::json newsJson;
            jsonInputStream >> newsJson;
            for (nlohmann::json::iterator it = newsJson.begin(); it != newsJson.end(); ++it) {
                auto entry = it.value();
                News r = jsonToNews(entry);
                if (r.regnrs == "") {
                    withoutRegnrsCount++;
                    if (r.title != "") {
                        std::clog << "Title without regnrs: " << r.title << std::endl;
                    }
                    continue;
                }
                auto regnrs = r.regnrs;
                if (regnrsToNews.find(regnrs) == regnrsToNews.end()) {
                    std::vector<News> v = { r };
                    regnrsToNews[regnrs] = v;
                } else {
                    auto v = regnrsToNews[regnrs];
                    v.push_back(r);
                    regnrsToNews[regnrs] = v;
                }
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

    News jsonToNews(nlohmann::json entry) {
        News news;
        news.title = entry["title"].get<std::string>();
        news.date = entry["date"].get<std::string>();
        news.description = entry["desc"].get<std::string>();
        news.pdfLink = entry["pdf"].get<std::string>();
        for (nlohmann::json::iterator it = entry["prep"].begin(); it != entry["prep"].end(); ++it) {
            auto prep = it.value();
            std::string prop = prep["prop"].get<std::string>();
            if (prop == "Zulassungsnummer" || prop == "No d'autorisation") {
                news.regnrs = prep["field"].get<std::string>();
            } else {
                try {
                    std::string val = prep["field"].get<std::string>();
                    news.extras[prop] = val;
                } catch(...){}
            }
        }
        return news;
    }
    
    std::vector<News> getNewsByRegnrs(std::string regnrs) {
        if (regnrsToNews.find(regnrs) != regnrsToNews.end()) {
            return regnrsToNews[regnrs];
        }
        std::vector<News> empty;
        return empty;

    }

    static void printFileStats(const std::string &filename) {
        REP::html_h2("DHPC HPC");

        REP::html_p(filename);
        
        REP::html_start_ul();
        int64_t count = 0;
        for (auto entry : regnrsToNews) {
            count += entry.second.size();
        }
        REP::html_li("Count: " + std::to_string(count));
        REP::html_li("Without Zulassungsnummer: " + std::to_string(withoutRegnrsCount));
        REP::html_end_ul();
    }
}