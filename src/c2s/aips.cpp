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
#include <filesystem>
#include <fstream>
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
    std::vector<std::string> statsMissingContentHtml;

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

    if (statsMissingContentHtml.size() > 0) {
        REP::html_h3("Missing html files");
        REP::html_start_ul();
        for (auto s : statsMissingContentHtml)
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
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("MedicinalDocumentsBundles")) {

            if (v.first != "MedicinalDocumentsBundle") {
                continue;
            }

            if (v.second.get("Domain", "") != "Human") {
                continue;
            }

            // Mapping: https://github.com/zdavatz/cpp2sqlite/issues/252
            // For --pinfo, Add both fi and pi: https://github.com/zdavatz/cpp2sqlite/issues/271
            if (!(type == "fi" && v.second.get("Type", "") == "SmPC") &&
                !(type == "pi" && (v.second.get("Type", "") == "SmPC" || v.second.get("Type", "") == "PIL"))
            ) {
                continue;
            }

            BOOST_FOREACH(pt::ptree::value_type &attachedDocument, v.second) {
                if (attachedDocument.first != "AttachedDocument") {
                    continue;
                }
                std::string lan = attachedDocument.second.get("Language", "");

                if (lan != language) {
                    continue;
                }

                Medicine Med;
                Med.title = attachedDocument.second.get("Description", "");
                boost::replace_all(Med.title, "&#038;", "&"); // Issue #49

                Med.auth = v.second.get("Holder.Name", "");

                // There's no more Med.subst since #252

                std::vector<std::string> rnVector;
                {
                    BOOST_FOREACH(pt::ptree::value_type &regulatedAuthorization, v.second.get_child("RegulatedAuthorization")) {
                        if (regulatedAuthorization.first == "Identifier") {
                            std::string regnr = regulatedAuthorization.second.data();
                            rnVector.push_back(regnr);
                        }
                    }
                    Med.regnrs = boost::algorithm::join(rnVector, ", ");

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

                        Med.regnrs = boost::algorithm::join(rnVector, ", ");
                        int sizeAfter = rnVector.size();
                        if (sizeBefore != sizeAfter)
                            statsDuplicateRegnrsVec.push_back(Med.regnrs);
                    }
                }

                BOOST_FOREACH(pt::ptree::value_type &documentReference, attachedDocument.second) {
                    if (documentReference.first != "DocumentReference") continue;
                    if (documentReference.second.get("ContentType", "") != "text/html") continue;
                    std::string url = documentReference.second.get("Url", "");
                    std::string htmlFilename = std::filesystem::path(url).filename();
                    std::string downloadFolderPath = std::filesystem::path(filename).parent_path();
                    std::string htmlPath = downloadFolderPath + "/Refdata-AllHtml/" + htmlFilename;
                    if (std::filesystem::exists(htmlPath))
                    {
                        Med.contentHTMLPath = htmlPath;
                    } else {
                        statsMissingContentHtml.push_back("Regnrs: " + Med.regnrs + " html: " + htmlFilename);
                    }
                    break;
                }

                //std::cerr << "title: " << Med.title << ", atc: " << Med.atc << ", subst: " << Med.subst << std::endl;

                if (!Med.contentHTMLPath.empty()) {
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
