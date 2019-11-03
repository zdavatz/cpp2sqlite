//
//  gtin.hpp
//  cpp2sqlite, pharma, zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 24 Jan 2019
//

#ifndef gtin_hpp
#define gtin_hpp

#include <string>
#include <vector>

namespace GTIN
{
    // Keep each GTIN associated to the corresponding pack info string
    // for the purpose of matching them in the HTML barcode section
    struct oneFachinfoPackages
    {
        std::vector<std::string> name;
        std::vector<std::string> gtin;
    };

    char getGtin13Checksum(std::string gtin12);
    bool verifyGtin13Checksum(std::string gtin13);
    std::string padToLength(int lenght, std::string s);
}

#endif /* gtin_hpp */
