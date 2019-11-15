//
//  swissmedic.hpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 14 Nov 2019
//

#ifndef swissmedic_hpp
#define swissmedic_hpp

#include <set>

namespace SWISSMEDIC
{
//    struct dosageUnits {
//        std::string dosage;
//        std::string units;
//    };

    struct pharmaRow {
        std::string rn5;        // A
//        std::string dosageNr;   // B
        std::string code3;
        std::string gtin13;
//        std::string name;
//        std::string galenicForm;
//        std::string owner;
//        std::string itNumber;
//        std::string categoryMed;
        std::string categoryPack;
//        std::string regDate;
//        std::string validUntil;
//        std::string narcoticFlag;
//        dosageUnits du;
    };

    void parseXLXS(const std::string &filename, bool dumpHeader = false);
    std::string getCategoryPackByGtin(const std::string &gtin);
}

#endif /* swissmedic_hpp */
