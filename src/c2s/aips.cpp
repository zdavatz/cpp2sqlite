//
//  aips.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 16 Jan 2019
//

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "aips.hpp"
#include "atc.hpp"
#include "epha.hpp"
#include "swissmedic.hpp"
#include "peddose.hpp"
#include "report.hpp"

namespace pt = boost::property_tree;

namespace AIPS
{
    MedicineList medList;

    // Parse-phase stats
    unsigned int statsAtcFromEphaCount = 0;
    unsigned int statsAtcFromAipsCount = 0;
    unsigned int statsAtcFromSwissmedicCount = 0;

    unsigned int statsAtcTextFoundCount = 0;
    unsigned int statsPedTextFoundCount = 0;
    unsigned int statsAtcTextNotFoundCount = 0;
    std::vector<std::string> statsTitlesWithRnZeroVec;
    std::set<std::string> statsUniqueAtcSet; // Issue #70

    // Usage stats
    std::vector<std::string> statsDuplicateRegnrsVec;
    std::set<std::string> statsMissingImgAltSet;
    std::vector<std::string> statsTitlesWithInvalidATCVec;

void addStatsMissingAlt(const std::string &regnrs, const int sectionNumber)
{
    // TODO: add the section number to a set
    statsMissingImgAltSet.insert(regnrs);
}

void addStatsInvalidAtc(const std::string &title, const std::string &rns)
{
    statsTitlesWithInvalidATCVec.push_back("RN: <" + rns + "> " + ", title: <" + title + "> ");
}
    
static
void printFileStats(const std::string &filename,
                    const std::string &language,
                    const std::string &type)
{
    REP::html_h2("AIPS");
    REP::html_p(filename);
    
    REP::html_start_ul();
    REP::html_li("medicalInformation " + type + " " + language + " " + std::to_string(medList.size()));
    REP::html_end_ul();
    
    REP::html_h3("ATC codes " + std::to_string(statsAtcFromEphaCount + statsAtcFromAipsCount + statsAtcFromSwissmedicCount + statsTitlesWithInvalidATCVec.size()));
    REP::html_start_ul();
    REP::html_li("from aips: " + std::to_string(statsAtcFromAipsCount));
    REP::html_li("from swissmedic: " + std::to_string(statsAtcFromSwissmedicCount));
    REP::html_li("unique (from both aips and swissmedic): " + std::to_string(statsUniqueAtcSet.size()));
    REP::html_end_ul();

    if (statsTitlesWithInvalidATCVec.size() > 0) {
        REP::html_p("Titles with empty or invalid ATC code: " + std::to_string(statsTitlesWithInvalidATCVec.size()));
        REP::html_start_ul();
        for (auto s : statsTitlesWithInvalidATCVec)
            REP::html_li(s);
        
        REP::html_end_ul();
    }

    REP::html_h3("ATC code text");
    REP::html_start_ul();
    REP::html_li("from aips: " + std::to_string(statsAtcTextFoundCount));
    REP::html_li("from peddose: " + std::to_string(statsPedTextFoundCount));
    REP::html_li("no text found: " + std::to_string(statsAtcTextNotFoundCount));
    REP::html_end_ul();

    if (statsDuplicateRegnrsVec.size() > 0) {
        REP::html_h3("rgnrs that contained duplicates");
        REP::html_start_ul();
        for (auto s : statsDuplicateRegnrsVec)
            REP::html_li(s);

        REP::html_end_ul();
    }
    
    if (statsTitlesWithRnZeroVec.size() > 0) {
        REP::html_h3("titles with rn 00000 (ignored)");
        REP::html_start_ul();
        for (auto s : statsTitlesWithRnZeroVec)
            REP::html_li(s);
        
        REP::html_end_ul();
    }
}
    
void printUsageStats()
{
    if (statsMissingImgAltSet.size() > 0) {
        REP::html_h3("XML");
        REP::html_p("For these rgnrs, <img> has no \"alt\" attribute");
        REP::html_start_ul();
        for (auto s : statsMissingImgAltSet)
            REP::html_li(s);
        
        REP::html_end_ul();
    }
}

MedicineList & parseXML(const std::string &filename,
                        const std::string &language,
                        const std::string &type,
                        bool verbose)
{
    pt::ptree tree;
    
    try {
        std::clog << std::endl << "Reading AIPS XML" << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cerr << "Line: " << __LINE__ << " Error " << e.what() << std::endl;
    }
    
    std::clog << "Analyzing AIPS" << std::endl;

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
                    boost::replace_all(Med.title, "&#038;", "&"); // Issue #49
                    
                    Med.auth = v.second.get("authHolder", "");
                    
                    Med.subst = v.second.get("substances", "");

                    std::vector<std::string> rnVector;
                    {
                        Med.regnrs = v.second.get("authNrs", "");
                        boost::algorithm::split(rnVector, Med.regnrs, boost::is_any_of(", "), boost::token_compress_on);
                        
                        if (rnVector[0] == "00000")
                            statsTitlesWithRnZeroVec.push_back(Med.title);
#ifdef DEBUG
                        // Check that there are no non-numeric characters
                        // See HTML for rn 51908 ("Numéro d’autorisation 51'908")
                        if (Med.regnrs.find_first_not_of("0123456789, ") != std::string::npos)
                            std::clog
                            << basename((char *)__FILE__) << ":" << __LINE__
                            << ", rn: <" << Med.regnrs + ">"
                            << ", title: <" << Med.title + ">"
                            << std::endl;
#endif

                        int sizeBefore = rnVector.size();
                        if (sizeBefore > 1) {
                            // Make sure there are no duplicate rn (26395 SOLCOSERYL, 37397 VENTOLIN)
                            // Preferable not to sort, which would affect the default order of packages later on
                            // Skip sorting assuming duplicate elements are guaranteed to be consecutive
                            // std::sort( rnVector.begin(), rnVector.end() );
                            rnVector.erase( std::unique( rnVector.begin(), rnVector.end()), rnVector.end());

                            Med.regnrs = boost::algorithm::join(rnVector, ",");
                            int sizeAfter = rnVector.size();
                            if (sizeBefore != sizeAfter)
                                statsDuplicateRegnrsVec.push_back(Med.regnrs);
                        }
                    }
                    
#if 0
                    Med.atc = EPHA::getAtcFromSingleRn(rnVector[0]);
                    if (!Med.atc.empty()) {
                        statsAtcFromEphaCount++;
                    }
                    else
#endif
                    {
                        // Fallback 1
                        Med.atc = v.second.get("atcCode", ""); // These ATCs need to be cleaned up
                        ATC::validate(Med.regnrs, Med.atc);    // Clean up the ATCs
                        if (!Med.atc.empty()) {
                            statsAtcFromAipsCount++;
                        }
                        else {
                            // Fallback 2
                            Med.atc = SWISSMEDIC::getAtcFromFirstRn(rnVector[0]);
                            if (!Med.atc.empty()) {
                                statsAtcFromSwissmedicCount++;
                            }
                            else {
                                // Add it to the report
                                AIPS::addStatsInvalidAtc(Med.title, Med.regnrs);
                            }
                        }
                    }
                    
                    // Add ";" and localized text from 'atc_codes_multi_lingual.txt'
                    if (!Med.atc.empty()) {
#if 1 // Issue #70
                        if (boost::contains(Med.atc, ",")) {
                            std::vector<std::string> atcVector;
                            boost::algorithm::split(atcVector, Med.atc, boost::is_any_of(","));
                            for (auto a : atcVector)
                                statsUniqueAtcSet.insert(a);
                        }
                        else {
                            statsUniqueAtcSet.insert(Med.atc);
                        }
#endif
                        std::string atcText = ATC::getTextByAtcs(Med.atc);
                        if (!atcText.empty()) {
                            statsAtcTextFoundCount++;
                            Med.atc += ";" + atcText;
                        }
                        else {
                            // Fallback 1
                            atcText = PED::getTextByAtcs(Med.atc);
                            if (!atcText.empty()) {
                                statsPedTextFoundCount++;
                                Med.atc += ";" + atcText;
                            }
                            else {
                                statsAtcTextNotFoundCount++;
                                if (verbose) {
                                    std::clog
                                    << "[" << statsAtcTextNotFoundCount << "]"
                                    << " no text for ATC: <" << Med.atc << ">"
                                    << " (first rn: " << rnVector[0] << ")"
                                    << std::endl;
                                }
                            }
                        }
                    }
                    
                    //std::cerr << "remark: " << v.second.get("remark", "") << std::endl;
                    //std::cerr << "style: " << v.second.get("style", "") << std::endl; // unused

                    Med.content = v.second.get("content", "");
                    //std::cout << "Med.content: " << Med.content << std::endl;

                    //std::cerr << "title: " << Med.title << ", atc: " << Med.atc << ", subst: " << Med.subst << std::endl;

                    medList.push_back(Med);
                }
            }
        }

        printFileStats(filename, language, type);
    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error " << e.what() << std::endl;
    }
    
    return medList;
}

}
