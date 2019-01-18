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
#include "aips.hpp"

namespace pt = boost::property_tree;

namespace AIPS
{
    
MedicineList medList;

MedicineList & parseXML(const std::string &filename,
                        const std::string &language,
                        const std::string &type)
{
    pt::ptree tree;
    
    try {
        std::cerr << "Reading XML" << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cout << "Line: " << __LINE__ << "Error" << e.what() << std::endl;
    }
    
    std::cerr << "Analyzing" << std::endl;

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
                    Med.atc = v.second.get("atcCode", "");
                    Med.subst = v.second.get("substances", "");

                    //std::cerr << "title: " << Med.title << ", atc: " << Med.atc << ", subst: " << Med.subst << std::endl;

                    medList.push_back(Med);
                }
            }
        }
        //std::cout << "title count: " << List.size() << std::endl;  // 22056
    }
    catch (std::exception &e) {
        std::cout << basename((char *)__FILE__) << ":" << __LINE__ << ", Error" << e.what() << std::endl;
    }
    
    return medList;
}

}
