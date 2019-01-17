//
//  aips.cpp
//  cpp2sqlite
//
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

MedicineList & parseXML(const std::string &filename)
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
                //std::cerr << "Line: " << __LINE__ << ", ";
                Medicine Med;
                Med.title = v.second.get("title", "");
                Med.auth = v.second.get("authHolder", "");
                Med.atc = v.second.get("atcCode", "");
                Med.subst = v.second.get("substances", "");
#if 0
                std::cerr << "title: "   << Med.title;
                std::cerr << ", atc: "   << Med.atc;
                std::cerr << ", subst: " << Med.subst << std::endl;
#endif
                medList.push_back(Med);
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
