//
//  bag.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 23 Jan 2019
//

#include <set>
#include <iomanip>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "bag.hpp"
#include "gtin.hpp"
#include "swissmedic.hpp"

namespace pt = boost::property_tree;

namespace BAG
{
    PreparationList prepList;
    int statsTotalGtinCount = 0;

void parseXML(const std::string &filename,
              const std::string &language,
              bool verbose)
{
    pt::ptree tree;
    
    std::string lan = language;
    lan[0] = toupper(lan[0]);
    const std::string descriptionTag = "Description" + lan;
    const std::string nameTag = "Name" + lan;

    try {
        std::clog << std::endl << "Reading bag XML" << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cerr << "Line: " << __LINE__ << "Error" << e.what() << std::endl;
    }
    
    std::clog << "Analyzing bag" << std::endl;

    int statsPackCount = 0;
    int statsPackWithoutGtinCount = 0;
    int statsPackRecoveredGtinCount = 0;
    int statsPackNotRecoveredGtinCount = 0;
    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("Preparations")) {
            if (v.first == "Preparation") {

                Preparation prep;
                prep.name = v.second.get(nameTag, "");
                prep.name = boost::to_upper_copy<std::string>(prep.name);

                prep.description = v.second.get(descriptionTag, "");
                boost::algorithm::trim_right(prep.description);
                prep.description = boost::to_lower_copy<std::string>(prep.description);

                prep.swissmedNo = v.second.get("SwissmedicNo5", "");
                if (!prep.swissmedNo.empty())
                    prep.swissmedNo = GTIN::padToLength(5, prep.swissmedNo);

                prep.orgen = v.second.get("OrgGenCode", "");
                prep.sb20 = v.second.get("FlagSB20", "");

                // Each preparation has multiple packs (GTIN)
                BOOST_FOREACH(pt::ptree::value_type &p, v.second.get_child("Packs")) {
                    if (p.first == "Pack") {
                        Pack pack;
                        pack.description = p.second.get(descriptionTag, ""); // TODO: trim trailing spaces
                        boost::algorithm::trim_right(pack.description);
                        pack.description = boost::to_lower_copy<std::string>(pack.description);

                        pack.category = p.second.get("SwissmedicCategory", "");
                        pack.gtin = p.second.get("GTIN", "");
                        if (pack.gtin.empty()) {
                            statsPackWithoutGtinCount++;
                            // Calculate from SwissmedicNo8
                            std::string gtin8 = p.second.get("SwissmedicNo8", "");
                            if (!gtin8.empty()) {
                                statsPackRecoveredGtinCount++;
                                gtin8 = GTIN::padToLength(8, gtin8);
                                std::string gtin12 = "7680" + gtin8;
                                char checksum = GTIN::getGtin13Checksum(gtin12);
                                pack.gtin = gtin12 + checksum;
                            }
                            else {
                                statsPackNotRecoveredGtinCount++;
                                if (verbose) {
                                    std::cerr
                                    << basename((char *)__FILE__) << ":" << __LINE__
                                    << ", SwissmedicNo8 empty"
                                    //<< " for SwissmedicNo5: " << prep.swissmedNo
                                    << ", <" << nameTag << "> " << v.second.get(nameTag, "")
                                    << ", <" << descriptionTag << "> " << v.second.get(descriptionTag, "")
                                    << std::endl;
                                }
                            }
                        }
                        else
                            GTIN::verifyGtin13Checksum(pack.gtin);

                        pack.limitationPoints = p.second.get("PointLimitations.PointLimitation.Points", "");
                        pack.exFactoryPrice = formatPriceAsMoney(p.second.get("Prices.ExFactoryPrice.Price", ""));
                        pack.publicPrice = formatPriceAsMoney(p.second.get("Prices.PublicPrice.Price", ""));
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
                
                ItCode itCode;
                size_t maxLen = size_t(0);
                size_t minLen = size_t(INT_MAX);
                BOOST_FOREACH(pt::ptree::value_type &itc, v.second.get_child("ItCodes")) {
                    if (itc.first == "ItCode") {
                        
                        std::string code;
                        pt::ptree & attributes = itc.second.get_child("<xmlattr>");
                        BOOST_FOREACH(pt::ptree::value_type &att, attributes) {
                            //std::cerr << "attr 1st: " << att.first.data() << ", 2nd: " << att.second.data() << std::endl;

                            if (att.first == "Code") {
                                code = att.second.data();
                                size_t n = code.size();

                                // application_str - choose the one with the longest attribute "Code"
                                if (maxLen < n) {
                                    maxLen = n;
                                    itCode.application = itc.second.get(descriptionTag, "");
                                }

                                // tindex_str - choose the one with the longest attribute "Code"
                                if (minLen > n) {
                                    minLen = n;
                                    itCode.tindex = itc.second.get(descriptionTag, "");
                                }
                            }
                        }
                    }
                } // BOOST_FOREACH ItCodes
                
#if 0
                static int i=0;
                if (i<10)
                    std::clog
                    << basename((char *)__FILE__) << ":" << __LINE__
                    << ", i:" << ++i
                    << ", tindex_str: " << itCode.tindex
                    << ", application_str: " << itCode.application
                    << std::endl;
#endif
                prep.itCodes = itCode;

                prepList.push_back(prep);
            }
        }
        
        std::cout << "bag preparations: " << prepList.size()
        << ", packs: " << statsPackCount
        << ", packs without GTIN: " << statsPackWithoutGtinCount
        << ", recovered GTIN: " << statsPackRecoveredGtinCount
        << ", not recovered GTIN: " << statsPackNotRecoveredGtinCount
        << std::endl;
    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error" << e.what() << std::endl;
    }
}

std::string getAdditionalNames(const std::string &rn,
                               std::set<std::string> &gtinUsed)
{
    std::string names;
    int i=0;
    std::set<std::string>::iterator it;

    for (Preparation pre : prepList) {
        if (rn != pre.swissmedNo)
            continue;

        for (Pack p : pre.packs) {
            std::string g13 = p.gtin;
            // TODO build gtin if missing
            it = gtinUsed.find(g13);
            if (it == gtinUsed.end()) { // not found in list of used GTINs, we must add the name
                statsTotalGtinCount++;
                gtinUsed.insert(g13);   // also update the list of GTINs used so far
                if (i++ > 0)
                    names += "\n";

                std::string name = pre.name + " " + pre.description;
                name += ", " + p.description;
            
#if 0
                std::cout << basename((char *)__FILE__) << ":" << __LINE__
                << " rn: " << rn
                << " FOUND " << name
                << std::endl;
#endif

#ifdef DEBUG_IDENTIFY_NAMES
                names += "bag+";
#endif
                names += name;

                std::string paf = getPricesAndFlags(g13, "", p.category);
                if (!paf.empty())
                    names += paf;
            }
        }
    }

    return names;
}

std::string getPricesAndFlags(const std::string &gtin,
                              const std::string &fromSwissmedic,
                              const std::string &category)
{
    std::string prices;
    std::vector<std::string> flagsVector;
    bool found = false;

    for (Preparation pre : prepList)
        for (Pack p : pre.packs)
            if (gtin == p.gtin) {
                if (!p.exFactoryPrice.empty())
                    prices += "EFP " + p.exFactoryPrice;

                if (!p.publicPrice.empty())
                    prices += ", PP " + p.publicPrice;

                if (!category.empty())
                    flagsVector.push_back(category);

                if (!p.exFactoryPrice.empty() || !p.publicPrice.empty())
                    flagsVector.push_back("SL");

                if (!p.limitationPoints.empty())
                    flagsVector.push_back("LIM" + p.limitationPoints);

                if (pre.sb20 == "Y")
                    flagsVector.push_back("SB 20%");
                else if (pre.sb20 == "N")
                    flagsVector.push_back("SB 10%");

                if (!pre.orgen.empty())
                    flagsVector.push_back(pre.orgen);

                found = true;
                goto prepareResult; // abort the two for loops
            }

prepareResult:
    // The category (input parameter) must be added even if the GTIN was not found
    if (!found) {
        if (!category.empty())
            flagsVector.push_back(category);

//        std::clog
//        << basename((char *)__FILE__) << ":" << __LINE__
//        << ", NOT FOUND: " << gtin
//        << std::endl;
    }
    
    std::string paf;
    if (!prices.empty())
        paf += ", " + prices;
    
    if (!fromSwissmedic.empty())
        paf += ", " + fromSwissmedic;

    std::string flagsString;
    if (flagsVector.size() > 0) {
        flagsString += " [";
        int i=0;
        for (auto f : flagsVector) {
            if (i++ > 0)
                flagsString += ", ";  // separator

            flagsString += f;
        }

        paf += flagsString + "]";
    }

    return paf;
}

std::vector<std::string> getGtinList()
{
    std::vector<std::string> list;

    for (Preparation pre : prepList)
        for (Pack p : pre.packs)
            if (!p.gtin.empty())
                list.push_back(p.gtin);

    return list;
}

std::string getTindex(const std::string &rn)
{
    std::string tindex;
    for (Preparation pre : prepList) {
        if (rn == pre.swissmedNo) {
            tindex = pre.itCodes.tindex;
            break;
        }
    }

    return tindex;
}
    
std::string getApplication(const std::string &rn)
{
    std::string app;
    for (Preparation p : prepList) {
        if (rn == p.swissmedNo) {
            app = p.itCodes.application + " (BAG)";
            break;
        }
    }

    return app;
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
    
void printStats()
{
    std::cout
    << "GTINs used from bag " << statsTotalGtinCount
    << std::endl;
}
}
