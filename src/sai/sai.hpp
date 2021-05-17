//
//  sai.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 13 May 2021
//

#ifndef sai_hpp
#define sai_hpp

namespace SAI
{
    struct _package {
        std::string approvalNumber;
        std::string sequenceNumber;
        std::string packageCode;
        std::string approvalStatus;
        std::string noteFreeText;
        std::string packageSize;
        std::string packageUnit;
        std::string revocationWaiverDate;
        std::string btmCode;
        std::string gtinIndustry;
        std::string inTradeDateIndustry;
        std::string outOfTradeDateIndustry;
        std::string descriptionEnRefdata;
        std::string descriptionFrRefdata;
    };

    void parseXML(const std::string &filename);

    std::vector<_package> getPackages();

    static void printFileStats(const std::string &filename);
}

#endif /* sai_hpp */
