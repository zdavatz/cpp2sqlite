//
//  ddd.hpp
//  pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 21 May 2019
//

#ifndef ddd_hpp
#define ddd_hpp

namespace DDD
{
    struct dddLine {
        std::string atc;
        
        // To be parsed from 3rd column
        double dailyDosage_mg;
        //std::string roa;
    };

    void parseCSV(const std::string &filename);
    bool getDailyDosage_mg_byATC(const std::string &atc,
                                 double *ddd_mg);
}

#endif
