//
//  stoffsynonyme.cpp
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
#include "stoffsynonyme.hpp"

namespace pt = boost::property_tree;

namespace STO
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
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("ns0:SMC_Synonyme")) {
            _package package;

            package.stoffId = v.second.get("STOFF_ID", "");
            package.synonymCode = v.second.get("SYNONYM_CODE", "");
            package.laufendeNr = v.second.get("LAUFENDE_NR", "");
            package.stoffsynonym = v.second.get("STOFFSYNONYM", "");
            package.quelle = v.second.get("QUELLE", "");
            package.sortierNr = v.second.get("SORTIER_NR", "");

            packagesMap[package.stoffId] = package;
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

_package getPackageByStoffId(std::string num) {
    return packagesMap.at(num);
}

static void printFileStats(const std::string &filename)
{
    // TODO
}

}
