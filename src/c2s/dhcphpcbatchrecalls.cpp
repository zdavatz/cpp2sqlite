//
//  dhcphpcbatchrecalls.cpp
//  dhcphpcbatchrecalls
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
#include "dhcphpcbatchrecalls.hpp"
#include "report.hpp"

namespace DHCPHPCBATCHRECALLS
{
    std::map<std::string, std::vector<Recall>> regnrsToRecalls;
    int64_t withoutRegnrsCount = 0;
    void parseJSON(const std::string &filename) {
        try {
            std::clog << std::endl << "Reading " << filename << std::endl;

            std::ifstream jsonInputStream(filename);
            nlohmann::json recallJson;
            jsonInputStream >> recallJson;
            for (nlohmann::json::iterator it = recallJson.begin(); it != recallJson.end(); ++it) {
                auto entry = it.value();
                Recall r = jsonToRecall(entry);
                if (r.regnrs == "") {
                    withoutRegnrsCount++;
                    if (r.title != "") {
                        std::clog << "Title without regnrs: " << r.title << std::endl;
                    }
                    continue;
                }
                for (auto regnrs : r.regnrsParsed) {
                    if (regnrsToRecalls.find(regnrs) == regnrsToRecalls.end()) {
                        std::vector<Recall> v = { r };
                        regnrsToRecalls[regnrs] = v;
                    } else {
                        auto v = regnrsToRecalls[regnrs];
                        v.push_back(r);
                        regnrsToRecalls[regnrs] = v;
                    }
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

    Recall jsonToRecall(nlohmann::json entry) {
        Recall recall;
        recall.title = entry["title"].get<std::string>();
        recall.date = entry["date"].get<std::string>();
        recall.description = entry["desc"].get<std::string>();
        recall.pdfLink = entry["pdf"].get<std::string>();
        for (nlohmann::json::iterator it = entry["prep"].begin(); it != entry["prep"].end(); ++it) {
            auto prep = it.value();
            std::string prop = prep["prop"].get<std::string>();
            if (prop == "Präparat" || prop == "Präparate" || prop == "Préparation" || prop == "Préparations") {
                recall.preparation = prep["field"].get<std::string>();
            }
            if (prop == "Zulassungsnummer"
                || prop == "Zulassungsnummern"
                || prop == "Zulasssungsnumer"
                || prop == "Zulaswsungsnummer"
                || prop == "No d'autorisation"
                || prop == "No d’autorisation"
                || prop == "No d’autorisation :"
                || prop == "Nos d’autorisation"
                || prop == "Nos. d'autorisation"
                || prop == "Nos. d’autorisation"
            ) {
                recall.regnrs = prep["field"].get<std::string>();
                std::vector<std::string> regnrsVector;
                boost::algorithm::split(regnrsVector, recall.regnrs, boost::is_any_of(", und"), boost::token_compress_on);
                recall.regnrsParsed = regnrsVector;
            }
            if (prop == "Wirkstoff" || prop == "Principe actif") {
                recall.substance = prep["field"].get<std::string>();
            }
            if (prop == "Zulassungsinhaberin" || prop == "Titulaire de l'autorisation" || prop == "Titulaire de l’autorisation") {
                recall.licensee = prep["field"].get<std::string>();
            }
            if (prop == "Rückzug der Charge" || prop == "Rückzug der Chargen" || prop == "Rückzug der Chargen:" || prop == "Retrait du lot" || prop == "Retrait des lots") {
                recall.withdrawalOfTheaBatch = prep["field"].get<std::string>();
            }
        }
        return recall;
    }
    
    std::vector<Recall> getRecallsByRegnrs(std::string regnrs) {
        if (regnrsToRecalls.find(regnrs) != regnrsToRecalls.end()) {
            return regnrsToRecalls[regnrs];
        }
        std::vector<Recall> empty;
        return empty;

    }

    static void printFileStats(const std::string &filename) {
        REP::html_h2("Dhcphpcbatchrecalls");

        REP::html_p(filename);
        
        REP::html_start_ul();
        int64_t count = 0;
        for (auto entry : regnrsToRecalls) {
            count += entry.second.size();
        }
        REP::html_li("Count: " + std::to_string(count));
        REP::html_li("Without Zulassungsnummer: " + std::to_string(withoutRegnrsCount));
        REP::html_end_ul();
    }
}