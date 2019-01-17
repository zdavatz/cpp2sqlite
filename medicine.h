//
//  medicine.h
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 16 Jan 2019
//

#ifndef medicine_h
#define medicine_h

#include <vector>

namespace AIPS
{
    
struct Medicine {
    std::string title;
    std::string auth;
    std::string atc;
    std::string subst;
};

typedef std::vector<Medicine> MedicineList;

}

#endif /* medicine_h */
