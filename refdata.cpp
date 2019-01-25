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
#include "gtin.hpp"
#include "bag.hpp"
#include "swissmedic.hpp"

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
        std::clog << std::endl << "Reading refdata XML" << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cerr << "Line: " << __LINE__ << "Error" << e.what() << std::endl;
    }
    
    std::clog << "Analyzing refdata" << std::endl;

    int statsArticleChildCount = 0;
    int statsItemCount = 0;
    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("ARTICLE")) {
            statsArticleChildCount++;
            if (v.first == "ITEM")
            {
                statsItemCount++;

                std::string atype = v.second.get("ATYPE", "");
                if (atype != "PHARMA")
                    continue;

                std::string gtin = v.second.get<std::string>("GTIN", "");

                // Check that GTIN starts with 7680
                std::string gtinPrefix = gtin.substr(0,4); // pos, len
                if (gtinPrefix != "7680") // 76=med, 80=Switzerland
                    continue;
                
                GTIN::verifyGtin13Checksum(gtin);

                Article article;
                article.gtin_13 = gtin;
                article.gtin_5 = gtin.substr(4,5); // pos, len
                article.name = v.second.get<std::string>(nameTag, "");
#if 1  // TBC
                static int k=0;
                if (article.name.empty())
                if (k++ < 13)
                    std::clog
                    << basename((char *)__FILE__) << ":" << __LINE__
                    << ", k:" << k
                    << ", gtin:" << gtin
                    << ", ev.nn.i.H."
                    << std::endl;
#endif

                artList.push_back(article);
            }
//            else {
//                // one "<xmlattr>" and one "RESULT"
//                std::cout << " v.first: " << v.first << std::endl;
//            }
        }

        std::cout
        << "refdata PHARMA items with GTIN starting with \"7680\", count: " << artList.size()
        << " of " << statsItemCount
        //<< " (" << statsArticleChildCount << " articles)"
        << std::endl;
    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error" << e.what() << std::endl;
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
            std::string cat = SWISSMEDIC::getCategoryFromGtin(art.gtin_13);
            std::string paf = BAG::getPricesAndFlags(art.gtin_13, cat);
            if (!paf.empty())
                names += paf;

            i++;
        }
    }
    
    return names;
}
    
bool findGtin(const std::string &gtin)
{
    for (Article art : artList) {
        if (art.gtin_13 == gtin)
            return true;
    }

    return false;
}

}
