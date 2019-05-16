//
//  swissmedic1.hpp
//  pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 22 Jan 2019
//

#ifndef swissmedic1_hpp
#define swissmedic1_hpp

#include <set>
//#include "gtin.hpp"

namespace SWISSMEDIC1
{
    struct dosageUnits {
        std::string dosage;
        std::string units;
    };

    struct pharmaRow {
        std::string rn5;        // A
        std::string dosageNr;   // B
        std::string code3;
        std::string gtin13;
        std::string name;
        std::string galenicForm;
        std::string owner;
        std::string itNumber;
        std::string categoryMed;
        std::string categoryPack;
        std::string regDate;
        std::string validUntil;
        std::string narcoticFlag;
        dosageUnits du;
    };

    void parseXLXS(const std::string &filename);
    
    void createCSV(const std::string &outDir);

//    int getAdditionalNames(const std::string &rn,
//                           std::set<std::string> &gtinUsed,
//                           GTIN::oneFachinfoPackages &packages,
//                           const std::string &language);
    int countRowsWithRn(const std::string &rn);
    std::string getApplication(const std::string &rn);
    std::string getAtcFromFirstRn(const std::string &rn);

    bool findGtin(const std::string &gtin);
    std::string getCategoryPackByGtin(const std::string &gtin);
    dosageUnits getByGtin(const std::string &gtin);

    void printUsageStats();
}

#endif /* swissmedic_hpp */
