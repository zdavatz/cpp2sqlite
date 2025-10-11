//
//  swissreg.cpp
//  swissreg
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 11 Oct 2025
//
// https://github.com/zdavatz/cpp2sqlite/issues/264


#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <map>
#include <libgen.h>     // for basename()

#include <boost/algorithm/string.hpp>
#include "swissreg.hpp"
#include "report.hpp"

namespace SWISSREG
{
    std::vector<Certificate> certificates;
    std::map<std::string, std::vector<Certificate>> regnrToCert;
    void parseJSON(const std::string &filename) {
        try {
            std::clog << std::endl << "Reading " << filename << std::endl;

            std::ifstream jsonInputStream(filename);
            nlohmann::json swissRegJson;
            jsonInputStream >> swissRegJson;
            for (nlohmann::json::iterator it = swissRegJson.begin(); it != swissRegJson.end(); ++it) {
                auto entry = it.value();
                Certificate c = jsonToCertificate(it.key(), entry);
                certificates.push_back(c);

                for (auto regnr : c.iksnrs) {
                    if (regnrToCert.find(regnr) == regnrToCert.end()) {
                        std::vector<Certificate> v = { c };
                        regnrToCert[regnr] = v;
                    } else {
                        auto v = regnrToCert[regnr];
                        v.push_back(c);
                        regnrToCert[regnr] = v;
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

    Certificate jsonToCertificate(std::string id, nlohmann::json entry) {
        Certificate cert;
        cert.certificateId = id;
        cert.certificateNumber = entry["certificateNumber"].get<std::string>();
        cert.issueDate = entry["issueDate"].get<std::string>();
        cert.publicationDate = entry["publicationDate"].get<std::string>();
        cert.registrationDate = entry["registrationDate"].get<std::string>();
        cert.protectionDate = entry["protectionDate"].get<std::string>();
        cert.basePatentDate = entry["basePatentDate"].get<std::string>();
        cert.basePatent = entry["basePatent"].get<std::string>();
        // cert.basePatentId = entry["basePatentId"].get<std::string>();
        cert.iksnrs = entry["iksnrs"].get<std::vector<std::string>>();
        cert.expiryDate = entry["expiryDate"].get<std::string>();
        cert.deletionDate = entry["deletionDate"].get<std::string>();
        return cert;
    }

    std::vector<Certificate> getCertsByRegnr(std::string regnr) {
        if (regnrToCert.find(regnr) != regnrToCert.end()) {
            return regnrToCert[regnr];
        }
        std::vector<Certificate> empty;
        return empty;
    }

    static void printFileStats(const std::string &filename) {
        REP::html_h2("Swissreg");

        REP::html_p(filename);

        REP::html_start_ul();
        REP::html_li("Count: " + std::to_string(certificates.size()));
        REP::html_end_ul();
    }
}

