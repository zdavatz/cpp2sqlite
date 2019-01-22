//
//  refdata.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 21 Jan 2019
//

#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "refdata.hpp"

namespace pt = boost::property_tree;

namespace REFDATA
{

    ArticleList artList;
    
    void parseXML(const std::string &filename,
                  const std::string &language)
{
    pt::ptree tree;
    const std::string nameTag = "NAME_" + boost::to_upper_copy( language );

    try {
        std::cerr << "Reading refdata XML" << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cout << "Line: " << __LINE__ << "Error" << e.what() << std::endl;
    }
    
    std::cerr << "Analyzing refdata" << std::endl;

    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("ARTICLE")) {
            if (v.first == "ITEM")
            {
                std::string atype = v.second.get("ATYPE", "");
                if (atype != "PHARMA")
                    continue;

                // Check that GTIN starts with 7680
                std::string gtin = v.second.get<std::string>("GTIN", "");
                std::string gtinPrefix = gtin.substr(0,4); // pos, len

                if (gtinPrefix != "7680") // 76=med, 80=Switzerland
                    continue;

                Article article;
                article.gtin_13 = gtin;
                article.gtin_5 = gtin.substr(4,5); // pos, len
                article.name = v.second.get<std::string>(nameTag, "");

                artList.push_back(article);
            }
        }

        std::cout << "refdata record count: " << artList.size() << std::endl;
    }
    catch (std::exception &e) {
        std::cout << basename((char *)__FILE__) << ":" << __LINE__ << ", Error" << e.what() << std::endl;
    }
}

// GTIN:
//  76      med
//  80      Swiss
//  12345   registration number
//  123     package (column K of swissmedic)
//  1       checksum
//
// Each registration number can have multiple packages.
// Get all of them, one per line
std::string getNames(const std::string &rn)
{
    std::string names;
    int i=0;
    for (Article art : artList) {
        if (art.gtin_5 == rn) {
            if (i>0)
                names += "\n";
            
            names += art.name;
            i++;
        }
    }
    
    return names;
}

}
