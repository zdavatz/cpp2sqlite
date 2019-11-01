//
//  atc.hpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 12 Mar 2019
//

#ifndef atc_hpp
#define atc_hpp

namespace ATC
{
    void parseTXT(const std::string &filename,
                  const std::string &language,
                  bool verbose);
    
    std::string getTextByAtc(const std::string atc);
}

#endif /* atc_hpp */
