//
//  peddose.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 15 Feb 2019
//

#include <iostream>
#include <set>
#include <map>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "sai.hpp"

namespace pt = boost::property_tree;

namespace SAI
{

std::vector<_package> packagesVec;

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
            // if (v.first == "Cases") {
            //     statsCasesCount = v.second.size();
            // }
            // else if (v.first == "Indications") {
            //     statsIndicationsCount = v.second.size();
            // }
            // else if (v.first == "Codes") {
            //     statsCodesCount = v.second.size();
            // }
            // else if (v.first == "Dosages") {
            //     statsDosagesCount = v.second.size();
            // }
        }  // FOREACH

        // i = 0;
        // BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SwissPedDosePublicationV3.Cases")) {
        //     if (v.first == "Case") {
        //         caseCaseIDSet.insert(v.second.get("CaseID", ""));
        //         caseAtcCodeSet.insert(v.second.get("ATCCode", ""));
        //         caseRoaCodeSet.insert(v.second.get("ROACode", ""));

        //         _case ca;
        //         ca.caseId = v.second.get("CaseID", "");
        //         ca.atcCode = v.second.get("ATCCode", "");
        //         ca.indicationKey = v.second.get("IndicationKey", "");
        //         ca.RoaCode = v.second.get("ROACode", "");
        //         caseVec.push_back(ca);
        //     }
        // } // FOREACH Cases
    } // try
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", Error " << e.what()
        << std::endl;
    }
}
}
