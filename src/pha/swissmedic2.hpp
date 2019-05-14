//
//  swissmedic2.hpp
//  pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 10 May 2019
//

#ifndef swissmedic2_hpp
#define swissmedic2_hpp

namespace SWISSMEDIC2
{
    struct pharmaExtraRow {
        std::string rn5;        // A
        std::string dosageNr;   // B
        std::string authType;   // E
    };

    void parseXLXS(const std::string &filename);
    std::string getAuthorizationByAtc(const std::string &atc, const std::string &dn);
}
#endif
