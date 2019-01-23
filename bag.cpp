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
        std::clog << std::endl << "Reading bag XML" << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cerr << "Line: " << __LINE__ << "Error" << e.what() << std::endl;
    }
    
    std::clog << "Analyzing bag" << std::endl;

    int statsPackCount = 0;
    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("Preparations")) {
            if (v.first == "Preparation") {

                Preparation prep;
                prep.orgen = v.second.get("OrgGenCode", "");
                prep.sb20 = v.second.get("FlagSB20", "");

                // Each preparation has multiple packs (GTIN)
                BOOST_FOREACH(pt::ptree::value_type &p, v.second.get_child("Packs")) {
                    if (p.first == "Pack") {
                        Pack pack;
                        pack.gtin = p.second.get("GTIN", "");
                        pack.exFactoryPrice = p.second.get("Prices.ExFactoryPrice.Price", "");
                        pack.publicPrice = p.second.get("Prices.PublicPrice.Price", "");
                        prep.packs.push_back(pack);
                        
                        statsPackCount++;
#if 0
                    static int i=0;
                    if (i<10)
                        std::clog
                        << basename((char *)__FILE__) << ":" << __LINE__
                        << ", i:" << ++i
                        << ", GTIN: " << pack.gtin
                        << ", EFP " << pack.exFactoryPrice
                        << ", PP " << pack.publicPrice
                        << std::endl;
#endif
                    }
                }

                prepList.push_back(prep);
            }
        }
        
        std::cout << "bag preparations: " << prepList.size() << ", packs (GTIN): " << statsPackCount << std::endl;
    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error" << e.what() << std::endl;
    }
}

std::string getFlags(const std::string &gtin_13)
{
    std::string flags;
    for (Preparation p : prepList) {
//        if (gtin_13 == p.gtin_13) {
//            flags += "[";
//            if (p.orgen == "O")
//                flags += "SO";
//            flags += "]";
//        }
    }

    return flags;
}

std::vector<std::string> getGtinList()
{
    std::vector<std::string> list;

    for (Preparation pre : prepList) {
        for (Pack p : pre.packs)
            list.push_back(p.gtin);
    }

    return list;
}

}
