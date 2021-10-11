//
//  adressen.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 16 May 2021
//

#ifndef adressen_hpp
#define adressen_hpp

namespace ADR
{
    struct _package {
        std::string partnerNr;
        std::string firmenname;
        std::string adresszeile1;
        std::string adresszeile2;
        std::string landCode;
        std::string plz;
        std::string ort;
        std::string sprachCode;
        std::string kanton;
        std::string glnRefdata;
    };

    void parseXML(const std::string &filename);

    _package getPackageByPartnerNr(std::string num);

    static void printFileStats(const std::string &filename);
}

#endif /* adressen_hpp */
