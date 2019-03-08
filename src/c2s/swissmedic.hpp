//
//  swissmedic.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 22 Jan 2019
//

#ifndef swissmedic_hpp
#define swissmedic_hpp

#include <set>
#include "gtin.hpp"

namespace SWISSMEDIC
{
    struct dosageUnits {
        std::string dosage;
        std::string units;
    };
    
    void parseXLXS(const std::string &filename);

    int getAdditionalNames(const std::string &rn,
                           std::set<std::string> &gtinUsed,
                           GTIN::oneFachinfoPackages &packages,
                           const std::string &language);
    int countRowsWithRn(const std::string &rn);
    std::string getApplication(const std::string &rn);
    std::string getAtcFromFirstRn(const std::string &rn);

    bool findGtin(const std::string &gtin);
    std::string getCategoryByGtin(const std::string &gtin);
    dosageUnits getByGtin(const std::string &gtin);

    void printUsageStats();
}

#endif /* swissmedic_hpp */
