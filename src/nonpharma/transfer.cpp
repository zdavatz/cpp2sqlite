//
//  transfer.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 9 Nov 2023
//

#include <iostream>
#include <fstream>
#include <string>
// #include <vector>
// #include <regex>
#include <map>

#include <boost/algorithm/string.hpp>

#include "transfer.hpp"

// #include <libgen.h>     // for basename()

namespace TRANSFER
{
std::map<std::string, TRANSFER::Entry> entries;

void parseDAT(const std::string &filename) {
    std::cout << "Reading " << filename << std::endl;

    std::ifstream file(filename);

    std::string str;
    while (std::getline(file, str))
    {
        boost::algorithm::trim_right_if(str, boost::is_any_of(" \n\r"));
        
        TRANSFER::Entry entry;
        entry.pharma_code = str.substr(3, 7);
        
        std::string ean13 = str.substr(83, 13);
        boost::algorithm::trim_left_if(ean13, boost::is_any_of("0"));
        entry.ean13 = ean13;

        std::string pexf = str.substr(60, 6);
        std::string ppub = str.substr(66, 6);
        double pexf_f = std::stod(pexf);
        double ppub_f = std::stod(ppub);
        entry.price = pexf_f / 100.0f;
        entry.pub_price = pexf_f / 100.0f;

        std::cout << "entry.pharma_code: " << entry.pharma_code << " ean: " << entry.ean13 << " price: " << entry.price << std::endl;
        entries.insert(std::make_pair(ean13, entry));
    }
}

TRANSFER::Entry getEntryWithGtin(const std::string &gtin)
{
    std::string str = gtin;
    boost::algorithm::trim_left_if(str, boost::is_any_of("0"));
    return entries[str];
}
}
