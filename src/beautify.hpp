//
//  beautify.hpp
//  cpp2sqlite, pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 29 Jan 2019
//

#ifndef beautify_hpp
#define beautify_hpp

#include <iostream>
#include "gtin.hpp"

namespace BEAUTY
{
    void beautifyName(std::string &name);
    //void sort(std::string &packInfo, std::set<std::string> &gtinUsed);
    void sort(GTIN::oneFachinfoPackages &packages);
    
    void cleanupForNonHtmlUsage(std::string &xml);
    void cleanupXml(std::string &xml,
                    const std::string regnrs);
}

#endif /* beautify_hpp */
