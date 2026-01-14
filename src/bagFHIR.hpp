//
//  bag.hpp
//  cpp2sqlite, pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 23 Jan 2019
//

#ifndef bagfhir_hpp
#define bagfhir_hpp

#include <iostream>
#include <set>
#include <map>
#include "gtin.hpp"

#include <nlohmann/json.hpp>

#include <bag.hpp>

namespace BAGFHIR
{
    typedef std::vector<BAG::Preparation> PreparationList;
    typedef std::map<std::string, BAG::packageFields> PackageMap;

    void parseNDJSON(const std::string &filename,
                     const std::string &language,
                     bool verbose);

    BAG::Preparation jsonToPreparation(nlohmann::json json, const std::string &language);

    int getAdditionalNames(const std::string &rn,
                           std::set<std::string> &gtinUsed,
                           GTIN::oneFachinfoPackages &packages);

    std::string getPricesAndFlags(const std::string &gtin,
                                  const std::string &fromSwissmedic,
                                  const std::string &category="");

    std::vector<std::string> getGtinList();



    // Things available in BAG preparation XML, but not in FHIR:
    // std::string getTindex(const std::string &rn);
    // std::string getApplicationByRN(const std::string &rn);
    // std::string getLongestItCodeByGtin(const std::string &gtin);

    std::string formatPriceAsMoney(double price);

    BAG::packageFields getPackageFieldsByGtin(const std::string &gtin);
    bool getPreparationAndPackageByGtin(const std::string &gtin, BAG::Preparation *outPrep, BAG::Pack *outPack);

    std::vector<std::string> gtinWhichDoesntStartWith7680();
    PreparationList getPrepList();

    void printUsageStats();
}

#endif /* bagfhir_hpp */
