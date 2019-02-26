//
//  aips.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 16 Jan 2019
//

#ifndef aips_hpp
#define aips_hpp

#include "medicine.h"

namespace AIPS
{
    MedicineList & parseXML(const std::string &filename,
                            const std::string &language,
                            const std::string &type,
                            bool verbose);
    void addStatsMissingAlt(const std::string &regnrs,
                            const int sectionNumber);
    void printUsageStats();
}

#endif /* aips_hpp */
