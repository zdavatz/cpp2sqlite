//
//  gtin.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 24 Jan 2019
//

#ifndef gtin_hpp
#define gtin_hpp

namespace GTIN
{
    
char getGtin13Checksum(std::string gtin12);
bool verifyGtin13Checksum(std::string gtin13);
std::string padToLength(int lenght, std::string s);
}

#endif /* gtin_hpp */
