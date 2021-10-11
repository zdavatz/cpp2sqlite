//
//  deklarationen.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 16 May 2021
//
// This files handle SAI-deklarationen.XML

#include <iostream>
#include <set>
#include <map>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "report.hpp"
#include "deklarationen.hpp"

namespace pt = boost::property_tree;

namespace DEK
{

std::map<std::string, std::vector<_package>> packagesMap;

void parseXML(const std::string &filename)
{

    pt::ptree tree;

    try {
        std::clog << std::endl << "Reading " << filename << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cerr << "Line: " << __LINE__ << " Error " << e.what() << std::endl;
    }

    int i=0;
    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SAI_DEKLARATIONEN")) {
            _package package;

            if (v.first == "DEKLARATION") {
                package.zulassungsnummer = v.second.get("ZULASSUNGSNUMMER", "");
                package.sequenznummer = v.second.get("SEQUENZNUMMER", "");
                package.komponentennummer = v.second.get("KOMPONENTENNUMMER", "");
                package.komponente = v.second.get("KOMPONENTE", "");
                package.zeilennummer = v.second.get("ZEILENNUMMER", "");
                package.sortierungZeilennummer = v.second.get("SORTIERUNG_ZEILENNUMMER", "");
                package.zeilentyp = v.second.get("ZEILENTYP", "");
                package.stoffId = v.second.get("STOFF_ID", "");
                package.stoffkategorie = v.second.get("STOFFKATEGORIE", "");
                package.menge = v.second.get("MENGE", "");
                package.mengenEinheit = v.second.get("MENGEN_EINHEIT", "");
                package.deklarationsart = v.second.get("DEKLARATIONSART", "");

                if (packagesMap.find(package.zulassungsnummer) == packagesMap.end()) {
                    std::vector<_package> packages = { package };
                    packagesMap[package.zulassungsnummer] = packages;
                } else {
                    auto v = packagesMap[package.zulassungsnummer];
                    v.push_back(package);
                    packagesMap[package.zulassungsnummer] = v;
                }
            }
        }  // FOREACH

    } // try
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", Error " << e.what()
        << std::endl;
    }
    printFileStats(filename);
}

std::vector<_package> getPackagesByZulassungsnummerAndSequenznummer(std::string zulassungsnummer, std::string sequenznummer) {
    std::vector<_package> packages;
    try {
        std::vector<_package> ps = packagesMap.at(zulassungsnummer);
        for (auto p : ps) {
            if (p.sequenznummer == sequenznummer) {
                packages.push_back(p);
            }
        }
    } catch (std::out_of_range e) {
    }
    return packages;
}

static void printFileStats(const std::string &filename)
{
    // TODO
}

}
