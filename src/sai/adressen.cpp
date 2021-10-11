//
//  adressen.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 16 May 2021
//
// This files handle SAI-Adressen.XML

#include <iostream>
#include <set>
#include <map>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "report.hpp"
#include "adressen.hpp"

namespace pt = boost::property_tree;

namespace ADR
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
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SAI_ADRESSEN")) {
            if (v.first == "ADRESSE") {
                _package package;
                package.partnerNr = v.second.get("PARTNER_NR", "");
                package.firmenname = v.second.get("FIRMENNAME", "");
                package.adresszeile1 = v.second.get("ADRESSZEILE_1", "");
                package.adresszeile2 = v.second.get("ADRESSZEILE_2", "");
                package.landCode = v.second.get("LAND_CODE", "");
                package.plz = v.second.get("PLZ", "");
                package.ort = v.second.get("ORT", "");
                package.sprachCode = v.second.get("SPRACH_CODE", "");
                package.kanton = v.second.get("KANTON", "");
                package.glnRefdata = v.second.get("GLN_REFDATA", "");

                packagesMap[package.partnerNr] = package;
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

_package getPackageByPartnerNr(std::string num) {
    return packagesMap.at(num);
}

static void printFileStats(const std::string &filename)
{
    // TODO
}

}
