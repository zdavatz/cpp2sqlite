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
    std::string auth;   // authHolder
    std::string atc;    // atcCode
    std::string subst;  // substances
    std::string regnrs; // comma separated list of registration numbers
    // atc_class ?

    std::string style;
    std::string contentHTMLPath; // Path to the XHTML file
    std::string sections;
};

typedef std::vector<Medicine> MedicineList;

}

#endif /* medicine_h */
