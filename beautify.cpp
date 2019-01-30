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

std::string beautifyName(std::string &name)
{
    char separator = ' ';
    
    // Uppercase the first word (up to the first space)
    std::string::size_type pos1 = name.find(separator);
    auto token1 = name.substr(0, pos1); // pos, len
    token1 = boost::to_upper_copy<std::string>(token1);
    
    // Lowercase the rest
    auto token2 = name.substr(pos1+1); // pos, len
    token2 = boost::to_lower_copy<std::string>(token2);
    
    return token1 + separator + token2;
}

// First packages with price
// then packages without price
std::string sort(std::string &packInfo)
{
    std::string s;
    
    std::vector<std::string> allLines;
    std::vector<std::string> linesWithPrice;
    std::vector<std::string> linesWithoutPrice;
    boost::algorithm::split(allLines, packInfo, boost::is_any_of("\n"), boost::token_compress_on);
    if (allLines.size() < 2)
        return packInfo;
    
    std::regex r(", EFP ");
    
    // Analyze
    for (auto line : allLines)
        if (std::regex_search(line, r))
            linesWithPrice.push_back(line);
        else
            linesWithoutPrice.push_back(line);
    
    // TODO: sort by galenic form each of the two vectors

    // Recombine all lines into a single string
    int linesCount = 0;
    for (auto l : linesWithPrice) {
        if (linesCount++ > 0)
            s += "\n";

        s += l;
    }

    for (auto l : linesWithoutPrice) {
        if (linesCount++ > 0)
            s += "\n";
        
        s += l;
    }
    
    return s;
}
    
void sortByGalenicForm(std::vector<std::string> &group)
{
        
}

}
