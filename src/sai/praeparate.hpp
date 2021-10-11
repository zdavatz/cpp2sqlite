//
//  praeparate.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 16 May 2021
//

#ifndef praeparate_hpp
#define praeparate_hpp

namespace PRA
{
    struct _package {
        std::string verwendung;
        std::string zulassungsnummer;
        std::string praeparatename;
        std::string arzneiform;
        std::string atcCode;
        std::string heilmittelCode;
        std::string zulassungsstatus;
        std::string zulassungskategorie;
        std::string zulassungsinhaberin;
        std::string erstzulassungsdatum;
        std::string basisZulassungsnummer;
        std::string abgabekategorie;
        std::string itNummer;
        std::string anwendungsgebiet;
        std::string ablaufdatum;
        std::string ausstellungsdatum;
        std::string chargenblockadeAktiv;
        std::string chargenfreigabePflicht;
        std::string einzeleinfuhrBewilligPflicht;
    };

    void parseXML(const std::string &filename);

    _package getPackageByZulassungsnummer(std::string num);

    static void printFileStats(const std::string &filename);
}

#endif /* praeparate_hpp */
