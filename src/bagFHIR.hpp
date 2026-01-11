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

namespace BAGFHIR
{
    struct packageFields {
        std::string efp;
        std::string efp_validFrom;
        std::string pp;
        std::vector<std::string> flags;
    };

    // struct ItCode {
    //     // shortest
    //     std::string tindex;         // localized

    //     // longest
    //     std::string application;    // localized
    //     std::string longestItCode;
    // };

    struct Pack {
        std::string description;
    //     std::string category;
        std::string gtin;
        std::string exFactoryPrice;
        std::string exFactoryPriceValidFrom;
        std::string publicPrice;
    };

    struct Bundle {
        std::string name;
        // std::string description;
        std::string regnr;
        std::string atcCode;
        std::string orgen;
        // std::string sb20;
        // std::string sb;
        std::vector<Pack> packs;
        // ItCode itCodes;
    };

    typedef std::vector<Bundle> BundleList;
    typedef std::map<std::string, packageFields> PackageMap;

    void parseNDJSON(const std::string &filename,
                     const std::string &language,
                     bool verbose);

    Bundle jsonToBundle(nlohmann::json json, const std::string &language);

    int getAdditionalNames(const std::string &rn,
                           std::set<std::string> &gtinUsed,
                           GTIN::oneFachinfoPackages &packages);

    std::string getPricesAndFlags(const std::string &gtin,
                                  const std::string &fromSwissmedic,
                                  const std::string &category="");

    std::vector<std::string> getGtinList();



    // N/A:
    // std::string getTindex(const std::string &rn);
    // std::string getApplicationByRN(const std::string &rn);
    // std::string getLongestItCodeByGtin(const std::string &gtin);
    // SwissmedicCategory

    std::string formatPriceAsMoney(double price);

    // packageFields getPackageFieldsByGtin(const std::string &gtin);

    // PreparationList getPrepList();

    // void printUsageStats();
}

#endif /* bagfhir_hpp */
