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

// Fool boost parser so it doesn't create children for "<sub>" and "<sup>" and "<br />"
#define WORKAROUND_SUB_SUP_BR
#define ESCAPED_SUB_L         "[[[[sub]]]]"  // "&lt;sub&gt;" would be altered by boost parser
#define ESCAPED_SUB_R         "[[[[/sub]]]]"
#define ESCAPED_SUP_L         "[[[[sup]]]]"
#define ESCAPED_SUP_R         "[[[[/sup]]]]"
#define ESCAPED_BR            "[[[[br]]]]"   // Issue #30, rn 66547, section20, French

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
