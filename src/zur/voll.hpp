//
//  voll.hpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 3 Nov 2019
//

#ifndef voll_hpp
#define voll_hpp

namespace VOLL
{
void parseCSV(const std::string &filename,
              const std::string type,
              bool dumpHeader = true);

void openDB(const std::string &filename);
void createDB();
void closeDB();
}

#endif /* voll_hpp */
