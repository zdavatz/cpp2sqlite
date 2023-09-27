//
//  bag.cpp
//  cpp2sqlite, pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 23 Jan 2019
//

#include <set>
#include <iomanip>
#include <sstream>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "bag.hpp"

namespace pt = boost::property_tree;

namespace BAG
{
    PackageMap packMap;

void parseXML(const std::string &filename,
              bool verbose)
{
    std::clog << std::endl << "Reading " << filename << std::endl;

    pt::ptree tree;

    try {
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cerr << "Line: " << __LINE__ << " Error " << e.what() << std::endl;
    }
    
#ifdef DEBUG
    std::clog << "Analyzing bag" << std::endl;
#endif

    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("Preparations")) {
            if (v.first == "Preparation") {
                // Each preparation has multiple packs (GTIN)
                BOOST_FOREACH(pt::ptree::value_type &p, v.second.get_child("Packs")) {
                    if (p.first == "Pack") {
                        Pack pack;
                        pack.gtin = p.second.get("GTIN", "");
                        if (pack.gtin.empty()) {
                            // Calculate from SwissmedicNo8
                            std::string gtin8 = p.second.get("SwissmedicNo8", "");
                            if (!gtin8.empty()) {
                                gtin8 = GTIN::padToLength(8, gtin8);
                                std::string gtin12 = "7680" + gtin8;
                                char checksum = GTIN::getGtin13Checksum(gtin12);
                                pack.gtin = gtin12 + checksum;
                            }
                            else {
#ifdef DEBUG
                                if (verbose) {
                                    std::cerr
                                    << basename((char *)__FILE__) << ":" << __LINE__
                                    << ", SwissmedicNo8 empty"
                                    << std::endl;
                                }
#endif
                            }
                        }
                        else
                            GTIN::verifyGtin13Checksum(pack.gtin);

                        pack.exFactoryPrice = formatPriceAsMoney(p.second.get("Prices.ExFactoryPrice.Price", ""));
                        pack.exFactoryPriceValidFrom = p.second.get("Prices.ExFactoryPrice.ValidFromDate", "");
                        pack.publicPrice = formatPriceAsMoney(p.second.get("Prices.PublicPrice.Price", ""));
                        pack.publicPriceValidFrom = p.second.get("Prices.PublicPrice.ValidFromDate", "");
                        pack.ggsl = p.second.get("FlagGGSL", "");
                        packMap[pack.gtin] = pack;
                    }
                }
            }
        }
    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error " << e.what() << std::endl;
    }
}

// Make sure the price string has only two decimal digits
std::string formatPriceAsMoney(const std::string &price)
{
    if (price.empty())
        return price;

    float f = std::stof(price);
    std::ostringstream s;
    s << std::fixed << std::setprecision(2) << f;
    return s.str();
}

Pack getPackageFieldsByGtin(const std::string &gtin)
{
    return packMap.at(gtin);
}

}
