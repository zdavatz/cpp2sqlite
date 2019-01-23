//
//  bag.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 23 Jan 2019
//

#ifndef bag_hpp
#define bag_hpp

#include <iostream>

namespace BAG
{
    struct Pack {
        std::string gtin;
        std::string exFactoryPrice;
        std::string publicPrice;
    };

    struct Preparation {
        std::string orgen;
        std::string sb20;
        std::vector<Pack> packs;
    };
    
    typedef std::vector<Preparation> PreparationList;
    
    void parseXML(const std::string &filename);
    std::string getFlags(const std::string &gtin_13);
    std::vector<std::string> getGtinList();
}

#endif /* bag_hpp */
