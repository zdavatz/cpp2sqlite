//
//  neu.hpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 19 Nov 2019
//

#ifndef neu_hpp
#define neu_hpp

namespace NEU
{
void parseCSV(const std::string &filename, bool dumpHeader = false);

void createConditionsNewJSON(const std::string &filename);
}

#endif /* neu_hpp */
