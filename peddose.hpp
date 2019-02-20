//
//  peddose.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 15 Feb 2019
//

#ifndef peddose_hpp
#define peddose_hpp

namespace PED
{
    struct _cases {
        std::string caseId;
        //std::string atcCode;
        std::string indicationKey;
        std::string RoaCode;
    };
    
    struct _codes {
        std::string description;    // TODO: localize
        std::string recStatus;
    };

    void parseXML(const std::string &filename,
                  const std::string &language);

    _cases getCaseByAtc(const std::string &atc);
    std::string getDescriptionByAtc(const std::string &atc);
}

#endif /* peddose_hpp */
