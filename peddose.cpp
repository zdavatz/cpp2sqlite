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

#include "peddose.hpp"

namespace pt = boost::property_tree;

namespace PED
{
    struct codeAtc {
        std::string description;    // TODO: localize
        std::string recStatus;
    };
    
    std::map<std::string, codeAtc> codeAtcMap; // key is CodeValue

    unsigned int statsCasesCount = 0;
    unsigned int statsIndicationsCount = 0;
    unsigned int statsCodesCount = 0;
    unsigned int statsDosagesCount = 0;

    unsigned int statsCode_ALTERRELATION = 0;
    unsigned int statsCode_FG = 0;
    unsigned int statsCode_GEWICHT = 0;
    unsigned int statsCodeAtc = 0;
    unsigned int statsCodeDOSISTYP = 0;
    unsigned int statsCodeDOSISUNIT = 0;
    unsigned int statsCodeEVIDENZ = 0;
    unsigned int statsCodeRoa = 0;
    unsigned int statsCodeZEIT = 0;
    
    std::set<std::string> caseCaseID;
    std::set<std::string> caseAtcCode;
    std::set<std::string> caseRoaCode;

    //std::set<std::string> codeAtcCode;
    std::set<std::string> codeRoaCode;

    std::set<std::string> dosageCaseID;

void parseXML(const std::string &filename,
              const std::string &language)
{
    pt::ptree tree;
    
    try {
        std::clog << std::endl << "Reading Ped XML" << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cerr << "Line: " << __LINE__ << " Error " << e.what() << std::endl;
    }
    
    std::clog << "Analyzing Ped" << std::endl;
    int i=0;

    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SwissPedDosePublication")) {
            if (v.first == "Cases") {
                statsCasesCount = v.second.size();
            }
            else if (v.first == "Indications") {
                statsIndicationsCount = v.second.size();
            }
            else if (v.first == "Codes") {
                statsCodesCount = v.second.size();
            }
            else if (v.first == "Dosages") {
                statsDosagesCount = v.second.size();
            }
        }  // FOREACH
        
        i = 0;
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SwissPedDosePublication.Cases")) {
            if (v.first == "Case") {
                /*
                 <ATCCode>J01CA04</ATCCode>
                 <IndicationKey>339</IndicationKey>
                 */
#if 0

                std::clog
                << basename((char *)__FILE__) << ":" << __LINE__
                << ", i: " << i++
                << ", # children: " << v.second.size()
                //<< ", key <" << v.first.data() << ">"       // Case
                << ", CaseTitle <" << v.second.get("CaseTitle", "") << ">"
                << std::endl;
#endif
                caseCaseID.insert(v.second.get("CaseID", ""));
                caseAtcCode.insert(v.second.get("ATCCode", ""));
                caseRoaCode.insert(v.second.get("ROACode", ""));
            }
        } // FOREACH Cases

        i = 0;
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SwissPedDosePublication.Indications")) {
            if (v.first == "Indication") {
                
                /* D German, F French, E English
                 
                 <IndicationKey>641</IndicationKey>
                 <IndikationNameE>Treatment of mild to moderately severe, acute pain</IndikationNameE>
                 */
#if 0

                std::clog
                << basename((char *)__FILE__) << ":" << __LINE__
                << ", i: " << i++
                << ", # children: " << v.second.size()
                << ", IndicationNameD <" << v.second.get("IndicationNameD", "") << ">"
                << std::endl;
#endif
            }
        } // FOREACH Indications

        i = 0;
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SwissPedDosePublication.Codes")) {
            if (v.first == "Code") {
                /*
                 <CodeType>ROA</CodeType>
                 <CodeValue>PO</CodeValue>
                 
                 <CodeType>ATC</CodeType>
                 <CodeValue>N02BA01</CodeValue>
                 <DecriptionE>Acetylsalicylsäure, inkl. Lysinacetylsalicylat</DecriptionE>
                 */
                std::string codeType = v.second.get("CodeType", "");
                std::string codeValue = v.second.get("CodeValue", "");
                if (codeType == "_ALTERRELATION")
                    statsCode_ALTERRELATION++;
                else if (codeType == "_FG")
                    statsCode_FG++;
                else if (codeType == "ATC") {
                    if (codeValue == "N02BA01")
                        std::clog << "\n\t ATC N02BA01 at: " << statsCodeAtc << std::endl;

                    statsCodeAtc++;
                    codeAtc ca {v.second.get("DescriptionD", ""),  // TODO: localize
                                v.second.get("RecStatus", "")};
                    codeAtcMap.insert(std::make_pair(v.second.get("CodeValue", ""), ca));
                }
                else if (codeType == "DOSISTYP")
                    statsCodeDOSISTYP++;
                else if (codeType == "DOSISUNIT")
                    statsCodeDOSISUNIT++;
                else if (codeType == "EVIDENZ")
                    statsCodeEVIDENZ++;
                else if (codeType == "ROA") {
                    statsCodeRoa++;
                    codeRoaCode.insert(v.second.get("CodeValue", ""));
                }
                else if (codeType == "ZEIT")
                    statsCodeZEIT++;

#if 0
                std::clog
                << basename((char *)__FILE__) << ":" << __LINE__
                << ", i: " << i++
                << ", # children: " << v.second.size()
                << ", CodeValue <" << v.second.get("CodeValue", "") << ">"
                << std::endl;
#endif
            }
        } // FOREACH Codes

        i = 0;
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SwissPedDosePublication.Dosages")) {
            if (v.first == "Dosage") {
                
#if 0
                std::clog
                << basename((char *)__FILE__) << ":" << __LINE__
                << ", i: " << i++
                << ", # children: " << v.second.size()
                << ", AgeFrom <" << v.second.get("AgeFrom", "") << ">"
                << ", AgeTo <" << v.second.get("AgeTo", "") << ">"
                << ", DoseRangeUnit <" << v.second.get("DoseRangeUnit", "") << ">"
                << std::endl;
#endif
                dosageCaseID.insert(v.second.get("CaseID", ""));

            }
        } // FOREACH Dosages
    } // try
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", Error " << e.what()
        << std::endl;
    }
    
    std::clog
    << basename((char *)__FILE__) << ":" << __LINE__
    << std::endl
    << "Cases: " << statsCasesCount
    << ", # CaseID: " << caseCaseID.size()
    << ", # ATC Code: " << caseAtcCode.size()
    << ", # ROA Code: " << caseRoaCode.size()
    << std::endl
    << "Indications: " << statsIndicationsCount
    << std::endl
    << "Dosages: " << statsDosagesCount
    << ", CaseID size: " << dosageCaseID.size()
    << std::endl
    << "Codes: " << statsCodesCount
    << ", # ATCCode: " << codeAtcMap.size()
    << ", # ROA Code: " << codeRoaCode.size()
    << std::endl
    << "  <CodeType>\n\t_ALTERRELATION: " << statsCode_ALTERRELATION
    << "\n\t_FG: " << statsCode_FG
    << "\n\t_GEWICHT: " << statsCode_GEWICHT
    << "\n\tATC: " << statsCodeAtc
    << "\n\tDOSISTYP: " << statsCodeDOSISTYP
    << "\n\tDOSISUNIT: " << statsCodeDOSISUNIT
    << "\n\tEVIDENZ: " << statsCodeEVIDENZ
    << "\n\tROA: " << statsCodeRoa
    << "\n\tZEIT: " << statsCodeZEIT
    << std::endl;
}
    
std::string getDescriptionByAtc(const std::string &atc)
{
    auto ca = codeAtcMap[atc];
    return ca.description;
}

}
