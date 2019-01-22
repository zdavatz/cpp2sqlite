//
//  swissmedic.hpp
//  cpp2sqlite
//
//  Created by Alex Bettarini on 22 Jan 2019
//

#ifndef swissmedic_hpp
#define swissmedic_hpp

#include <stdio.h>

namespace SWISSMEDIC
{
    void parseXLXS(const std::string &filename);
    std::string getName(const std::string &rn);
}

#endif /* swissmedic_hpp */
