//
//  bag.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 23 Jan 2019
//

#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include "bag.hpp"

namespace pt = boost::property_tree;

namespace BAG
{

void parseXML(const std::string &filename)
{
    pt::ptree tree;
    
    try {
        std::cerr << "Reading bag XML" << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cout << "Line: " << __LINE__ << "Error" << e.what() << std::endl;
    }
    
    try {
        int i=0;
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("Preparations")) {
            if (v.first == "Preparation") {
                std::cerr << ++i
                << ", OrgGenCode: " << v.second.get("OrgGenCode", "")
                << ", FlagSB20: " << v.second.get("FlagSB20", "")
                << ", GTIN: " << v.second.get("Packs.Pack.GTIN", "")
                << ", EFP " << v.second.get("Packs.Pack.Prices.ExFactoryPrice.Price", "")
                << ", PP " << v.second.get("Packs.Pack.Prices.PublicPrice.Price", "")
                << std::endl;
            }
        }
    }
    catch (std::exception &e) {
        std::cout << basename((char *)__FILE__) << ":" << __LINE__ << ", Error" << e.what() << std::endl;
    }
}
    
}
