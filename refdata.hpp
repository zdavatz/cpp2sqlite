//
//  refdata.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 21 Jan 2019
//

#ifndef refdata_hpp
#define refdata_hpp

#include <iostream>
#include <vector>

#include "medicine.h"

namespace REFDATA
{
    struct Article {
        std::string gtin_13;
        std::string gtin_5;
        std::string name;
    };
    
    typedef std::vector<Article> ArticleList;
    
    void parseXML(const std::string &filename,
                  const std::string &language);

    std::string getNames(const std::string &rn);
}

#endif /* refdata_hpp */
