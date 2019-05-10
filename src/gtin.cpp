//
//  gtin.cpp
//  cpp2sqlite, pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 24 Jan 2019
//

#include <iostream>
#include <libgen.h>     // for basename()
#include "gtin.hpp"

namespace GTIN
{

// https://www.gs1.org/services/how-calculate-check-digit-manually
// See Utilities.java, line 41, function getChecksum
char getGtin13Checksum(std::string gtin12or13)
{
    int n = gtin12or13.length();

    if (n < 12) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__
        << ", GTIN too short: " << gtin12or13
        << std::endl;
        return '0';
    }
    
//    std::cout
//    << basename((char *)__FILE__) << ":" << __LINE__
//    << ", gtin12: " << gtin12or13
//    << ", len:" << n;
    
    int val=0;
    
    for (int i=0; i<12; i++) {
        val += (gtin12or13[i] - '0') * ((i%2==0)?1:3);
    }
    
    int checksum_digit = 10 - (val % 10);
    if (checksum_digit == 10)
        checksum_digit = 0;
    
//    std::cout
//    << ", checksum_digit:" << checksum_digit
//    << std::endl;
    
    return checksum_digit + '0';
}

bool verifyGtin13Checksum(std::string gtin13)
{
    if (gtin13.empty())
        return false;

    char checksum = getGtin13Checksum(gtin13);
    
    if (checksum != gtin13[12]) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", GTIN error, expected:" << checksum
        << ", received" << gtin13[12]
        << std::endl;

        return false;
    }
    
    return true;
}
    
// Pad with leading zeros
std::string padToLength(int lenght, std::string s)
{
    std::string padded = s;
    while (padded.length() < lenght)
        padded = "0" + padded;
    
    return padded;
}

}
