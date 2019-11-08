//
//  stamm.hpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 1 Nov 2019
//

#ifndef stamm_hpp
#define stamm_hpp

struct stockStruct {
    int zurrose {0};
    int voigt {0};
};

namespace STAMM
{
void parseCSV(const std::string &filename, bool dumpHeader = false);
void parseVoigtCSV(const std::string &filename, bool dumpHeader = false);

void createStockCSV(const std::string &filename);
}

#endif /* stamm_hpp */
