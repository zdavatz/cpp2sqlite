//
//  peddose.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 15 Feb 2019
//

#include <iostream>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "peddose.hpp"

namespace pt = boost::property_tree;

namespace PED
{
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

void parseXML(const std::string &filename)
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
                }
                else if (codeType == "DOSISTYP")
                    statsCodeDOSISTYP++;
                else if (codeType == "DOSISUNIT")
                    statsCodeDOSISUNIT++;
                else if (codeType == "EVIDENZ")
                    statsCodeEVIDENZ++;
                else if (codeType == "ROA")
                    statsCodeRoa++;
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
    << ", Cases: " << statsCasesCount
    << ", Indications: " << statsIndicationsCount
    << ", Codes: " << statsCodesCount
    << ", Dosages: " << statsDosagesCount
    << std::endl
    << "CodeType\n\t_ALTERRELATION: " << statsCode_ALTERRELATION
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

}
