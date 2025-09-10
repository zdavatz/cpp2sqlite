//
//  refdata.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 21 Jan 2019
//

#include <set>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <regex>

#include "refdata.hpp"
#include "gtin.hpp"
#include "bag.hpp"
#include "swissmedic.hpp"
#include "beautify.hpp"
#include "report.hpp"

namespace pt = boost::property_tree;

namespace REFDATA
{
    ArticleList artList;

    unsigned int statsArticleChildCount = 0;
    unsigned int statsItemCount = 0;

    unsigned int statsTotalGtinCount = 0;

static
void printFileStats(const std::string &filename)
{
    REP::html_h2("RefData");
    //REP::html_p(std::string(basename((char *)filename.c_str())));
    REP::html_p(filename);

    REP::html_start_ul();
    REP::html_li("PHARMA items: " + std::to_string(statsItemCount));
    REP::html_li("PHARMA items with GTIN starting with \"7680\": " + std::to_string(artList.size()));
    REP::html_li("articles: " + std::to_string(statsArticleChildCount));
    REP::html_end_ul();
}

void printUsageStats()
{
    REP::html_h2("RefData");

    REP::html_start_ul();
    REP::html_li("GTINs used: " + std::to_string(statsTotalGtinCount));
    REP::html_end_ul();
}

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
        std::cerr << "Line: " << __LINE__ << " Error " << e.what() << std::endl;
    }

    std::clog << "Analyzing refdata" << std::endl;

    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("Articles")) {
            statsArticleChildCount++;
            if (v.first == "Article")
            {
                statsItemCount++;

                pt::ptree &medicinalProduct = v.second.get_child("MedicinalProduct");
                pt::ptree &productClassification = medicinalProduct.get_child("ProductClassification");
                std::string atype = productClassification.get<std::string>("ProductClass", "");
                std::string atc = productClassification.get<std::string>("Atc", "");
                std::string authorisationIdentifier = medicinalProduct.get<std::string>("RegulatedAuthorisationIdentifier", "");

                if (atype != "PHARMA")
                    continue;

                pt::ptree &packagedProduct = v.second.get_child("PackagedProduct");
                std::string gtin = packagedProduct.get<std::string>("DataCarrierIdentifier");

                pt::ptree &nameElement = packagedProduct.get_child("Name");

                // Check that GTIN starts with 7680
                std::string gtinPrefix = gtin.substr(0,4); // pos, len
                if (gtinPrefix != "7680") // 76=med, 80=Switzerland
                    continue;

                GTIN::verifyGtin13Checksum(gtin);

                Article article;
                article.gtin_13 = gtin;
                article.gtin_5 = gtin.substr(4,5); // pos, len
                article.phar = ""; // No pharma code
                article.name = nameElement.get<std::string>("FullName");
                article.atc = atc;
                article.authorisation_identifier = authorisationIdentifier;
                BEAUTY::beautifyName(article.name);

                artList.push_back(article);
            }
        }

        printFileStats(filename);
    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error " << e.what() << std::endl;
    }
}

// Each registration number can have multiple packages.
// Get all of them, one per line
// With the second argument we keep track of which GTINs have been used so far for this rn
// Return count added
int getNames(const std::string &rn,
             std::set<std::string> &gtinUsed,
             GTIN::oneFachinfoPackages &packages)
{
    int countAdded = 0;

    for (Article art : artList) {
        if (art.gtin_5 == rn) {
            countAdded++;
            statsTotalGtinCount++;

            std::string onePackageInfo;
#ifdef DEBUG_IDENTIFY_NAMES
            onePackageInfo += "ref+";
#endif
            onePackageInfo += art.name;

            std::string cat = SWISSMEDIC::getCategoryByGtin(art.gtin_13);
            std::string paf = BAG::getPricesAndFlags(art.gtin_13, "", cat);
            if (!paf.empty())
                onePackageInfo += paf;

            gtinUsed.insert(art.gtin_13);
            packages.gtin.push_back(art.gtin_13);
            packages.name.push_back(onePackageInfo);
        }
    }

    return countAdded;
}

bool findGtin(const std::string &gtin)
{
    for (Article art : artList)
        if (art.gtin_13 == gtin)
            return true;

    return false;
}

std::string findAtc(const std::string &regnrs) {
    for (Article art : artList) {
        // std::clog << "authorisation_identifier: " << art.authorisation_identifier << " size: " << std::to_string(art.authorisation_identifier.size()) << std::endl;
        // std::clog << "finding " << (art.authorisation_identifier.size() >= 5 ? art.authorisation_identifier.substr(0, 5) : "xx") << " with " << regnrs << std::endl;
        if (art.authorisation_identifier.size() >= 5 && art.authorisation_identifier.substr(0, 5) == regnrs) {
            return art.atc;
        }
    }
    return "";
}

std::string getPharByGtin(const std::string &gtin)
{
    std::string phar;

    for (Article art : artList)
        if (art.gtin_13 == gtin) {
            phar = art.phar;
            break;
        }

    return phar;
}

std::regex sectionIdRegex(R"(^section\d+$)");
void findSectionIdsAndTitle(
    pt::ptree tree,
    std::vector<std::string> &sectionIds,
    std::vector<std::string> &sectionTitles
) {
    BOOST_FOREACH(pt::ptree::value_type &v, tree) {
        std::string idAttr = v.second.get<std::string>("<xmlattr>.id", "");

        std::smatch match;
        if (std::regex_search(idAttr, match, sectionIdRegex)) {
            std::string sectionId = match[0];
            std::string sectionTitle = BEAUTY::getFlatPTreeContent(v.second);
            BEAUTY::cleanupForNonHtmlUsage(sectionTitle);
            sectionIds.push_back(sectionId);
            sectionTitles.push_back(sectionTitle);
        } else {
            findSectionIdsAndTitle(v.second, sectionIds, sectionTitles);
        }
    }
}

}
