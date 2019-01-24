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
#include <boost/algorithm/string.hpp>

#include "bag.hpp"
#include "gtin.hpp"

namespace pt = boost::property_tree;

namespace BAG
{

PreparationList prepList;

void parseXML(const std::string &filename,
              const std::string &language)
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
                prep.swissmedNo = v.second.get("SwissmedicNo5", "");
                prep.orgen = v.second.get("OrgGenCode", "");
                prep.sb20 = v.second.get("FlagSB20", "");

                // Each preparation has multiple packs (GTIN)
                BOOST_FOREACH(pt::ptree::value_type &p, v.second.get_child("Packs")) {
                    if (p.first == "Pack") {
                        Pack pack;
                        pack.gtin = p.second.get("GTIN", "");
                        if (pack.gtin.empty()) {
                            statsPackWithoutGtinCount++;
                            // Calculate from SwissmedicNo8
                            std::string gtin8 = p.second.get("SwissmedicNo8", "");
                            if (!gtin8.empty()) {
                                statsPackRecoveredGtinCount++;
                                gtin8 = GTIN::padToLength(8, gtin8);
                                char checksum = GTIN::getGtin13Checksum("7680" + gtin8);
                                pack.gtin = gtin8 + checksum;
                            }
                            else {
                                statsPackNotRecoveredGtinCount++;
                                std::cerr
                                << basename((char *)__FILE__) << ":" << __LINE__
                                << ", SwissmedicNo8 empty"
                                //<< " for SwissmedicNo5: " << prep.swissmedNo
                                << ", <" << nameTag << "> " << v.second.get(nameTag, "")
                                << ", <" << descriptionTag << "> " << v.second.get(descriptionTag, "")
                                << std::endl;
                            }
                        }
                        else
                            GTIN::verifyGtin13Checksum(pack.gtin);

                        pack.limitationPoints = p.second.get("PointLimitations.PointLimitation.Points", "");
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

std::string getPricesAndFlags(const std::string &gtin)
{
    std::string prices;
    std::vector<std::string> flagsVector;

    bool found = false;
    for (Preparation pre : prepList)
        for (Pack p : pre.packs) {
            if (gtin == p.gtin) {
                if (!p.exFactoryPrice.empty())
                    prices += "EFP " + p.exFactoryPrice;

                if (!p.publicPrice.empty())
                    prices += ", PP " + p.publicPrice;

                if (!p.exFactoryPrice.empty() || !p.publicPrice.empty())
                    flagsVector.push_back("SL");

                if (!p.limitationPoints.empty())
                    flagsVector.push_back("LIM" + p.limitationPoints);

                if (pre.sb20 == "Y")
                    flagsVector.push_back("SB 20%");
                else if (pre.sb20 == "N")
                    flagsVector.push_back("SB 10%");

                if (pre.orgen == "O")
                    flagsVector.push_back("O");
                else if (pre.orgen == "G")
                    flagsVector.push_back("G");
                

                found = true;
            } // if
        }

    std::string paf;
    if (!prices.empty())
        paf += prices;

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
    for (Preparation p : prepList) {
        if (rn == p.swissmedNo) {
            tindex = p.itCodes.tindex;
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
}
