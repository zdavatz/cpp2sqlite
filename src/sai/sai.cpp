//
//  sai.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 13 May 2021
//
// This files handle Typ1-Packungen.XML

#include <iostream>
#include <set>
#include <map>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "report.hpp"
#include "sai.hpp"

namespace pt = boost::property_tree;

namespace SAI
{

std::vector<_package> packagesVec;
std::set<std::string> approvalNumbersWithEmptyGTIN;

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
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("ns0:SMC_Packung")) {
            _package package;
            if (v.first == "PACKUNG") {
                package.approvalNumber = v.second.get("ZULASSUNGSNUMMER", "");
                package.sequenceNumber = v.second.get("SEQUENZNUMMER", "");
                package.packageCode = v.second.get("PACKUNGSCODE", "");
                package.approvalStatus = v.second.get("ZULASSUNGSSTATUS", "");
                package.noteFreeText = v.second.get("BEMERKUNG_FREITEXT", "");
                package.packageSize = v.second.get("PACKUNGSGROESSE", "");
                package.packageUnit = v.second.get("PACKUNGSEINHEIT", "");
                package.revocationWaiverDate = v.second.get("WIDERRUF_VERZICHT_DATUM", "");
                package.btmCode = v.second.get("BTM_CODE", "");
                package.gtinIndustry = v.second.get("GTIN_INDUSTRY", "");
                package.inTradeDateIndustry = v.second.get("IM_HANDEL_DATUM_INDUSTRY", "");
                package.outOfTradeDateIndustry = v.second.get("AUSSER_HANDEL_DATUM_INDUSTRY", "");
                package.descriptionEnRefdata = v.second.get("BESCHREIBUNG_DE_REFDATA", "");
                package.descriptionFrRefdata = v.second.get("BESCHREIBUNG_FR_REFDATA", "");
                packagesVec.push_back(package);

                if (package.gtinIndustry == "") {
                    approvalNumbersWithEmptyGTIN.insert(package.approvalNumber);
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

std::vector<_package> getPackages() {
    return packagesVec;
}

static void printFileStats(const std::string &filename)
{
    REP::html_h2("SAI");
    REP::html_p(filename);
    REP::html_start_ul();
    REP::html_li("rows: " + std::to_string(packagesVec.size()));
    REP::html_li("rows with empty GTIN: " + std::to_string(approvalNumbersWithEmptyGTIN.size()));
    REP::html_end_ul();
    REP::html_h2("rows with empty GTIN: ");
    REP::html_start_ul();
    for (auto num : approvalNumbersWithEmptyGTIN) {
        REP::html_li(num);
    }
    REP::html_end_ul();
}

}
