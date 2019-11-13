//
//  galen.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 12 Nov 2019
//

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include <libgen.h>     // for basename()

#include "galen.hpp"

namespace GALEN
{
std::map<int, std::string> galenicMap;

void parseTXT(const std::string &inDir)
{
    std::string filename = inDir + "/zurrose/galenic_codes_map_zurrose.txt";
    
    std::clog << std::endl << "Reading " << filename << std::endl;
    
    try {
        std::ifstream file(filename);
        
        std::string str;
        while (std::getline(file, str)) {
            
            if (str.length() == 0)
                continue; // skip empty lines

            const std::string separator1(" ");
            std::string::size_type pos1 = str.find(separator1);
            auto galenicCode = std::stoi(str.substr(0, pos1)); // pos, len
            auto galenicForm = str.substr(pos1+separator1.length());

            galenicMap.insert(std::make_pair(galenicCode, galenicForm));
        }

        file.close();
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
#ifdef DEBUG
    std::clog
    << "Parsed " << galenicMap.size() << " galenic codes"
    << std::endl;
#endif
}

std::string getTextByCode(int code)
{
//    std::string text;
//    auto search = galenicMap.find(code);
//    if (search != galenicMap.end())
//        text = search->second;
//
//    return text;
    return galenicMap[code];
}

}
