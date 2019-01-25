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

namespace SWISSMEDIC
{
    void parseXLXS(const std::string &filename);
    std::string getNames(const std::string &rn);
    std::string getAdditionalNames(const std::string &rn,
                                   const std::set<std::string> &gtinSet);
    int countRowsWithRn(const std::string &rn);
    bool findGtin(const std::string &gtin);
    std::string getApplication(const std::string &rn);
    std::string getCategoryFromGtin(const std::string &gtin);
    void printStats();
}

#endif /* swissmedic_hpp */