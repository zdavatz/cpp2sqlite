//
//  aips.hpp
//  pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 6 Jun 2019
//

#ifndef aips_hpp
#define aips_hpp

namespace AIPS
{
    void parseXML(const std::string &filename,
                            const std::string &language,
                            const std::string &type,
                            bool verbose);
    
    std::string getStorageByRN(const std::string &rn);
}

#endif /* aips_hpp */
