//
//  atc.cpp
//  interaction
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 12 Mar 2019
//

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <libgen.h>     // for basename()

#include "atc.hpp"

namespace ATC
{
    std::map<std::string, std::string> atcMap;

void parseTXT(const std::string &filename,
              const std::string &language,
              bool verbose)
{
    try {
        //std::clog << std::endl << "Reading atc TXT" << std::endl;
        std::ifstream file(filename);
        
        std::string str;
        while (std::getline(file, str)) {
            
            const std::string separator1(": ");
            std::string::size_type pos1 = str.find(separator1);
            auto atc = str.substr(0, pos1); // pos, len
            
            const std::string separator2("; ");
            std::string::size_type pos2 = str.find(separator2);
            auto textDe = str.substr(pos1+separator1.length(),       // pos
                                     pos2-pos1-separator1.length()); // len
            
            auto textFr = str.substr(pos2+separator2.length()); // pos, len
            
            atcMap.insert(std::make_pair(atc,
                                         language == "fr" ? textFr : textDe));
        }
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
}
    
// The input string is a single atc
std::string getTextByAtc(const std::string atc)
{
    std::string text;
    auto search = atcMap.find(atc);
    if (search != atcMap.end())
        text = search->second;
    
    return text;
}
}
