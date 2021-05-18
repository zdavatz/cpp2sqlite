//
//  sequenzen.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 16 May 2021
//

#ifndef sequenzen_hpp
#define sequenzen_hpp

namespace SEQ
{
    struct _package {
        std::string zulassungsnummer;
        std::string sequenznummer;
        std::string zulassungsstatus;
        std::string widerrufVerzichtDatum;
        std::string sequenzname;
        std::string zulassungsart;
        std::string basisSequenznummer;
        std::string anwendungsgebiet;
    };

    void parseXML(const std::string &filename);

    _package getPackagesByZulassungsnummerAndSequenznummer(std::string zulassungsnummer, std::string sequenznummer);

    static void printFileStats(const std::string &filename);
}

#endif /* sequenzen_hpp */
