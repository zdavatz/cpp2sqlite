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

PreparationList prepList;

void parseXML(const std::string &filename)
{
    pt::ptree tree;
    
    try {
        std::clog << "Reading bag XML" << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cerr << "Line: " << __LINE__ << "Error" << e.what() << std::endl;
    }
    
    std::clog << "Analyzing bag" << std::endl;

    int statsPrepCount = 0;
    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("Preparations")) {
            statsPrepCount++;
            if (v.first == "Preparation") {

                Preparation prep;
                prep.orgen = v.second.get("OrgGenCode", "");
                prep.sb20 = v.second.get("FlagSB20", "");
                prep.gtin13 = v.second.get("Packs.Pack.GTIN", "");
                prep.exFactoryPrice = v.second.get("Packs.Pack.Prices.ExFactoryPrice.Price", "");
                prep.publicPrice = v.second.get("Packs.Pack.Prices.PublicPrice.Price", "");

#if 0
                static int i=0;
                std::clog << ++i
                << ", OrgGenCode: " << prep.orgen
                << ", FlagSB20: " << prep.sb20
                << ", GTIN: " << prep.gtin13
                << ", EFP " << prep.exFactoryPrice
                << ", PP " << prep.publicPrice
                << std::endl;
#endif
                prepList.push_back(prep);
            }
        }
        
        std::cout << "bag preparations: " << prepList.size() << " of " << statsPrepCount << std::endl;

    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error" << e.what() << std::endl;
    }
}

std::string getFlags(const std::string &rn)
{
    return "";
}

}
