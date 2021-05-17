//
//  praeparate.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 16 May 2021
//
// This files handle Typ1-Praeparate.XML

#include <iostream>
#include <set>
#include <map>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "report.hpp"
#include "praeparate.hpp"

namespace pt = boost::property_tree;

namespace PRA
{

std::map<std::string, _package> packagesMap;

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
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("ns0:SMC_Praeparat")) {
            _package package;
            package.verwendung = v.second.get("VERWENDUNG", "");
            package.zulassungsnummer = v.second.get("ZULASSUNGSNUMMER", "");
            package.praeparatename = v.second.get("PRAEPARATENAME", "");
            package.arzneiform = v.second.get("ARZNEIFORM", "");
            package.atcCode = v.second.get("ATC_CODE", "");
            package.heilmittelCode = v.second.get("HEILMITTEL_CODE", "");
            package.zulassungsstatus = v.second.get("ZULASSUNGSSTATUS", "");
            package.zulassungskategorie = v.second.get("ZULASSUNGSKATEGORIE", "");
            package.zulassungsinhaberin = v.second.get("ZULASSUNGSINHABERIN", "");
            package.erstzulassungsdatum = v.second.get("ERSTZULASSUNGSDATUM", "");
            package.basisZulassungsnummer = v.second.get("BASIS_ZULASSUNGSNUMMER", "");
            package.abgabekategorie = v.second.get("ABGABEKATEGORIE", "");
            package.itNummer = v.second.get("IT_NUMMER", "");
            package.anwendungsgebiet = v.second.get("ANWENDUNGSGEBIET", "");
            package.ablaufdatum = v.second.get("ABLAUFDATUM", "");
            package.ausstellungsdatum = v.second.get("AUSSTELLUNGSDATUM", "");
            package.chargenblockadeAktiv = v.second.get("CHARGENBLOCKADE_AKTIV", "");
            package.chargenfreigabePflicht = v.second.get("CHARGENFREIGABE_PFLICHT", "");
            package.einzeleinfuhrBewilligPflicht = v.second.get("EINZELEINFUHR_BEWILLIG_PFLICHT", "");
            package.ocabrStandardCommon_name = v.second.get("OCABR_STANDARD_COMMON_NAME", "");
            packagesMap[package.zulassungsnummer] = package;
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

_package getPackageByZulassungsnummer(std::string num) {
    return packagesMap.at(num);
}

static void printFileStats(const std::string &filename)
{
    // TODO
}

}
