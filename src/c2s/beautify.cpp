//
//  beautify.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 29 Jan 2019
//

#include <string>
#include <sstream>
#include <regex>
#include <libgen.h>     // for basename()

#include <boost/algorithm/string.hpp>

#include "beautify.hpp"

namespace BEAUTY
{

void beautifyName(std::string &name)
{
    char separator = ' ';
    
    // Uppercase the first word (up to the first space)
    std::string::size_type pos1 = name.find(separator);
    auto token1 = name.substr(0, pos1); // pos, len
    token1 = boost::to_upper_copy<std::string>(token1);
    
    // Lowercase the rest
    auto token2 = name.substr(pos1+1); // pos, len
    token2 = boost::to_lower_copy<std::string>(token2);
    
    name = token1 + separator + token2;
}

// Sort package infos and gtins maintaining their pairing
// The sorting rule is
//      first packages with price
//      then packages without price
void sort(GTIN::oneFachinfoPackages &packages)
{
    if (packages.name.size() < 2)
        return;     // nothing to sort

#if 1 // Possibly redundant check now
    // For a couple of packages: 26395 SOLCOSERYL, and 37397 VENTOLIN
    // we have more pack info lines than gtins, because there are some
    // doubles pack info lines
    if (packages.name.size() != packages.gtin.size()) {
        std::cerr << std::endl
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", ERROR - pack info lines: " << packages.name.size()
        << ", gtin used: " << packages.gtin.size()
        << std::endl;

        for (auto line : packages.name)
            std::clog << "\tinfo " << line << std::endl;

        for (auto g : packages.gtin)
            std::clog << "\tgtin " << g << std::endl;
        
        return; // impossible to sort
    }
#endif

    // Start sorting, first by presence of price
    std::regex r(", EFP ");
    
    // Analyze
    std::vector<std::string> linesWithPrice;
    std::vector<std::string> linesWithoutPrice;

    std::vector<std::string> gtinsWithPrice;
    std::vector<std::string> gtinsWithoutPrice;
    std::vector<std::string>::iterator itGtin;

    itGtin = packages.gtin.begin();
    for (auto line : packages.name)
    {
        if (std::regex_search(line, r)) {
            linesWithPrice.push_back(line);
            gtinsWithPrice.push_back(*itGtin);
        }
        else {
            linesWithoutPrice.push_back(line);
            gtinsWithoutPrice.push_back(*itGtin);
        }
        
        itGtin++;
    }
    
    // TODO: sort by galenic form each of the two vectors

    // Prepare the results
    packages.name.clear();
    packages.gtin.clear();

    //std::string s;

    itGtin = gtinsWithPrice.begin();
    for (auto l : linesWithPrice) {
        packages.name.push_back(l);
        packages.gtin.push_back(*itGtin++);
    }

    itGtin = gtinsWithoutPrice.begin();
    for (auto l : linesWithoutPrice) {
        packages.name.push_back(l);
        packages.gtin.push_back(*itGtin++);
    }
}
    
void sortByGalenicForm(std::vector<std::string> &group)
{
        
}

}
