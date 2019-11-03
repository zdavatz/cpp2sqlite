//
//  kunden.hpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 1 Nov 2019
//

#ifndef kunden_hpp
#define kunden_hpp

namespace KUNDEN
{
void parseCSV(const std::string &filename);

void createConditionsJSON(const std::string &filename);
void createIdsJSON(const std::string &filename);
}

#endif /* kunden_hpp */
