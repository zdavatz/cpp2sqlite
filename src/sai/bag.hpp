//
//  bag.hpp
//  cpp2sqlite, pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 23 Jan 2019
//

#ifndef bag_hpp
#define bag_hpp

#include <iostream>
#include <set>
#include <map>
#include "gtin.hpp"

namespace BAG
{

    struct Pack {
        std::string gtin;
        std::string exFactoryPrice;
        std::string exFactoryPriceValidFrom;
        std::string publicPrice;
        std::string publicPriceValidFrom;
        std::string ggsl;
    };
    
    // typedef std::vector<Preparation> PreparationList;
    typedef std::map<std::string, Pack> PackageMap;

    void parseXML(const std::string &filename,
                  bool verbose);

    std::string formatPriceAsMoney(const std::string &price);

    Pack getPackageFieldsByGtin(const std::string &gtin);
}

#endif /* bag_hpp */
