//
//  sequenzen.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 16 May 2021
//
// This files handle SAI-Sequenzen.XML

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
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SAI_SEQUENZEN")) {
            if (v.first == "SEQUENZ") {
                _package package;

                package.zulassungsnummer = v.second.get("ZULASSUNGSNUMMER", "");
                package.sequenznummer = v.second.get("SEQUENZNUMMER", "");
                package.zulassungsstatus = v.second.get("ZULASSUNGSSTATUS", "");
                package.widerrufVerzichtDatum = v.second.get("WIDERRUF_VERZICHT_DATUM", "");
                package.sequenzname = v.second.get("SEQUENZNAME", "");
                package.zulassungsart = v.second.get("ZULASSUNGSART", "");
                package.basisSequenznummer = v.second.get("BASIS_SEQUENZNUMMER", "");
                package.anwendungsgebiet = v.second.get("ANWENDUNGSGEBIET", "");

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

_package getPackagesByZulassungsnummerAndSequenznummer(std::string zulassungsnummer, std::string sequenznummer) {
    for (auto p : packagesMap.at(zulassungsnummer)) {
        if (p.sequenznummer == sequenznummer) {
            return p;
        }
    }
    _package p;
    return p;
}

static void printFileStats(const std::string &filename)
{
    // TODO
}

}
