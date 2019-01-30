//
//  atc.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 30 Jan 2019
//

#ifndef atc_hpp
#define atc_hpp

namespace ATC
{
    void validate(const std::string &regnrs,
                  std::string &name);

    void printStats();
}

#endif /* atc_hpp */
