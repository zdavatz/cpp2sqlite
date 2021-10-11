//
//  stoffsynonyme.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 16 May 2021
//

#ifndef stoffsynonyme_hpp
#define stoffsynonyme_hpp

namespace STO
{
    struct _package {
        std::string stoffId;
        std::string synonymCode;
        std::string laufendeNr;
        std::string stoffsynonym;
        std::string quelle;
    };

    void parseXML(const std::string &filename);

    _package getPackageByStoffId(std::string num);

    static void printFileStats(const std::string &filename);
}

#endif /* stoffsynonyme_hpp */
