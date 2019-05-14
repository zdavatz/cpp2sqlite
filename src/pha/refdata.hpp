//
//  refdata.hpp
//  pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 21 Jan 2019
//

#ifndef refdata_hpp
#define refdata_hpp

#include <iostream>
#include <vector>

//#include "medicine.h"
#include "gtin.hpp"

namespace REFDATA
{
    struct Article {
        std::string gtin_13;
        std::string gtin_5;
        std::string phar;
        std::string name;
    };
    
    typedef std::vector<Article> ArticleList;
    
    void parseXML(const std::string &filename,
                  const std::string &language);

    int getNames(const std::string &rn,
                 std::set<std::string> &gtinUsed,
                 GTIN::oneFachinfoPackages &packages);

    bool findGtin(const std::string &gtin);

    //std::string getPharByGtin(const std::string &gtin);
    std::string getNameByGtin(const std::string &gtin);

    void printUsageStats();
}

#endif /* refdata_hpp */
