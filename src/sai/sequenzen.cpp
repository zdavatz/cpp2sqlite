//
//  sequenzen.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 16 May 2021
//
// This files handle Typ1-Sequenzen.XML

#include <iostream>
#include <set>
#include <map>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "report.hpp"
#include "sequenzen.hpp"

namespace pt = boost::property_tree;

namespace SEQ
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
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("ns0:SMC_Sequenz")) {
            _package package;

            package.zulassungsnummer = v.second.get("ZULASSUNGSNUMMER", "");
            package.sequenznummer = v.second.get("SEQUENZNUMMER", "");
            package.zulassungsstatus = v.second.get("ZULASSUNGSSTATUS", "");
            package.widerrufVerzichtDatum = v.second.get("WIDERRUF_VERZICHT_DATUM", "");
            package.sequenzname = v.second.get("SEQUENZNAME", "");
            package.zulassungsart = v.second.get("ZULASSUNGSART", "");
            package.basisSequenznummer = v.second.get("BASIS_SEQUENZNUMMER", "");
            package.anwendungsgebiet = v.second.get("ANWENDUNGSGEBIET", "");

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
