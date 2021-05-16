//
//  deklarationen.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 16 May 2021
//

#ifndef deklarationen_hpp
#define deklarationen_hpp

namespace DEK
{
    struct _package {
        std::string zulassungsnummer;
        std::string sequenznummer;
        std::string komponentennummer;
        std::string komponente;
        std::string zeilennummer;
        std::string sortierungZeilennummer;
        std::string zeilentyp;
        std::string stoffId;
        std::string stoffkategorie;
        std::string menge;
        std::string mengenEinheit;
        std::string deklarationsart;
    };

    void parseXML(const std::string &filename);

    std::vector<_package> getPackagesByZulassungsnummer(std::string num);

    static void printFileStats(const std::string &filename);
}

#endif /* deklarationen_hpp */
