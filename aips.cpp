//
//  aips.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 16 Jan 2019
//

#include <iostream>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "aips.hpp"
#include "atc.hpp"
#include "epha.hpp"
#include "swissmedic.hpp"

namespace pt = boost::property_tree;

namespace AIPS
{
    MedicineList medList;
    int statsAtcFromEphaCount = 0;
    int statsAtcFromAipsCount = 0;
    int statsAtcFromSwissmedicCount = 0;
    int statsAtcNotFoundCount = 0;

MedicineList & parseXML(const std::string &filename,
                        const std::string &language,
                        const std::string &type)
{
    pt::ptree tree;
    
    try {
        std::clog << std::endl << "Reading AIPS XML" << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cerr << "Line: " << __LINE__ << "Error" << e.what() << std::endl;
    }
    
    std::clog << "Analyzing AIPS" << std::endl;

    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("medicalInformations")) {
            
            if (v.first == "medicalInformation") {
                
                std::string typ;
                std::string lan;
                pt::ptree & attributes = v.second.get_child("<xmlattr>");
                BOOST_FOREACH(pt::ptree::value_type &att, attributes) {
                    //std::cerr << "attr 1st: " << att.first.data() << ", 2nd: " << att.second.data() << std::endl;
                    if (att.first == "type") {
                        typ = att.second.data();
                        //std::cerr << "Line: " << __LINE__ << ", type: " << type << std::endl;
                    }

                    if (att.first == "lang") {
                        lan = att.second.data();
                        //std::cerr << "Line: " << __LINE__ << ", language: " << language << std::endl;
                    }
                }

                if ((lan == language) && (typ == type)) {
                    Medicine Med;
                    Med.title = v.second.get("title", "");
                    Med.auth = v.second.get("authHolder", "");
                    
                    Med.subst = v.second.get("substances", "");
                    Med.regnrs = v.second.get("authNrs", "");
                    
                    std::vector<std::string> rnVector;
                    boost::algorithm::split(rnVector, Med.regnrs, boost::is_any_of(", "), boost::token_compress_on);
                    Med.atc = EPHA::getAtcFromSingleRn(rnVector[0]);
                    if (!Med.atc.empty()) {
                        statsAtcFromEphaCount++;
                    }
                    else {
                        // Fallback 1
                        Med.atc = v.second.get("atcCode", "");
                        ATC::validate(Med.regnrs, Med.atc); // these ones need to be cleaned up
                        if (!Med.atc.empty()) {
                            statsAtcFromAipsCount++;
                        }
                        else {
                            // Fallback 2
                            Med.atc = SWISSMEDIC::getAtcFromFirstRn(rnVector[0]);
                            if (!Med.atc.empty())
                                statsAtcFromSwissmedicCount++;
                            else
                                statsAtcNotFoundCount++;
                        }
                    }

                    //std::cerr << "remark: " << v.second.get("remark", "") << std::endl;
                    //std::cerr << "style: " << v.second.get("style", "") << std::endl; // unused

                    Med.content = v.second.get("content", "");
                    // TODO: change XML to HTML

                    //std::cerr << "sections: " << v.second.get("sections", "") << std::endl; // empty

                    //std::cerr << "title: " << Med.title << ", atc: " << Med.atc << ", subst: " << Med.subst << std::endl;

                    medList.push_back(Med);
                }
            }
        }
        
        std::cout
        << "aips medicalInformation " << type << " " << language << " " << medList.size()
        << std::endl
        << "ATC from epha: " << statsAtcFromEphaCount
        << ", from aips: " << statsAtcFromAipsCount
        << ", from swm: " << statsAtcFromSwissmedicCount
        << ", not found: " << statsAtcNotFoundCount
        << " (total " << (statsAtcFromEphaCount + statsAtcFromAipsCount + statsAtcFromSwissmedicCount + statsAtcNotFoundCount) << ")"
        << std::endl;
    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error" << e.what() << std::endl;
    }
    
    return medList;
}

}
