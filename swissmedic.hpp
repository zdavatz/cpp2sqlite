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

#include <stdio.h>

namespace SWISSMEDIC
{
    void parseXLXS(const std::string &filename);
    std::string getNames(const std::string &rn);
    int countRowsWithRn(const std::string &rn);
    bool findGtin(const std::string &gtin);
    std::string getApplication(const std::string &rn);
}

#endif /* swissmedic_hpp */
