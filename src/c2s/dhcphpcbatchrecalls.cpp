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
    std::map<std::string, Recall> regnrsToRecall;
    void parseJSON(const std::string &filename) {
        try {
            std::clog << std::endl << "Reading " << filename << std::endl;

            std::ifstream jsonInputStream(filename);
            nlohmann::json recallJson;
            jsonInputStream >> recallJson;
            for (nlohmann::json::iterator it = recallJson.begin(); it != recallJson.end(); ++it) {
                auto entry = it.value();
                Recall r = jsonToRecall(entry);
                regnrsToRecall[r.regnrs] = r;
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
            if (prop == "Präparat" || prop == "Préparation") {
                recall.preparation = prep["field"].get<std::string>();
            }
            if (prop == "Zulassungsnummer" || prop == "No d'autorisation") {
                recall.regnrs = prep["field"].get<std::string>();
            }
            if (prop == "Wirkstoff" || prop == "Principe actif") {
                recall.substance = prep["field"].get<std::string>();
            }
            if (prop == "Zulassungsinhaberin" || prop == "Titulaire de l'autorisation") {
                recall.licensee = prep["field"].get<std::string>();
            }
            if (prop == "Rückzug der Charge" || prop == "Retrait du lot") {
                recall.withdrawalOfTheaBatch = prep["field"].get<std::string>();
            }
        }
        return recall;
    }
    
    Recall getRecallByRegnrs(std::string regnrs) {
        if (regnrsToRecall.find(regnrs) != regnrsToRecall.end()) {
            return regnrsToRecall[regnrs];
        }
        Recall r;
        r.title = "";
        return r;

    }

    static void printFileStats(const std::string &filename) {
        REP::html_h2("Dhcphpcbatchrecalls");

        REP::html_p(filename);
        
        REP::html_start_ul();
        REP::html_li("Count : " + std::to_string(regnrsToRecall.size()));
        REP::html_end_ul();
    }
}