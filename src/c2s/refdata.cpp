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

std::set<std::string> getHtmlItalicClasses(std::string styleStr) {
    std::regex styleRegex(R"(\.(s\d+)\{([^}]*)\})");
    std::set<std::string> result;

    const std::sregex_iterator end;
    for (std::sregex_iterator i(styleStr.begin(), styleStr.end(), styleRegex); i != end; ++i)
    {
        std::string className = (*i)[1];
        std::string styleContent = (*i)[2];
        if (boost::contains(styleContent, "font-style:italic")) {
            result.insert(className);
        }
    }
    return result;
}

std::set<std::string> allClassesOfTree(pt::ptree tree) {
    std::set<std::string> result;
    std::string className = tree.get<std::string>("<xmlattr>.class", "");
    if (!className.empty()) {
        result.insert(className);
    }
    BOOST_FOREACH(pt::ptree::value_type &v, tree) {
        std::set<std::string> subResult = allClassesOfTree(v.second);
        result.insert(subResult.begin(), subResult.end());
    }
    return result;
}

ArticleDocument getArticleDocument(std::string path) {
    ArticleDocument document;
    std::vector<ArticleSection> sections;

    pt::ptree tree;
    try {
        std::ifstream inStream(path, std::ios::in | std::ios::binary);
        std::string xhtml = std::string((std::istreambuf_iterator<char>(inStream)), std::istreambuf_iterator<char>());

        BEAUTY::cleanupForNonHtmlUsage(xhtml);
        boost::replace_all(xhtml, "\ufeff", "");

        // Somehow the html files are so broken they has 2 xml declaration,
        // remove them all and add one back at last
        boost::replace_all(xhtml, "<?xml version=\"1.0\" encoding=\"utf-8\"?>", "");

        // Somehow the html files are so broken, it starts with <div> instead of <html>
        // but interestingly with </html>
        std::regex r1("^\\s*<div ");
        xhtml = std::regex_replace(xhtml, r1, "<html ");

        xhtml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>" + xhtml;

        std::stringstream ss;
        ss << xhtml;
        pt::read_xml(ss, tree/*, pt::xml_parser::no_concat_text*/);
    } catch (std::exception &e) {
        std::clog << basename((char *)__FILE__) << ":" << __LINE__ << ", Error in " << path << " " << e.what() << std::endl;
        return document;
    }
    BEAUTY::cleanUpSpan(tree);

    std::set<std::string> italicClasses;
    try {
        std::string styleStr = BEAUTY::getFlatPTreeContent(tree.get_child("html.head.style"));
        italicClasses = getHtmlItalicClasses(styleStr);
    } catch (std::exception &e) {
        // ignore
    }


    pt::ptree body;
    try {
        body = tree.get_child("html.body");
    } catch (std::exception &e) {
        std::clog << "Error1 in getArticleDocument!" << std::endl;
        {
            std::stringstream ss;
            pt::write_xml(ss, tree);
            std::string xml = ss.str();
            std::clog << "errored xml: " << xml << std::endl;
        }
        throw e;
    }
    ArticleSection currentSection;
    // We have to calculate our own section number,
    // as some html doesn't start with a section.
    int sectionNumber = 1;
    BOOST_FOREACH(pt::ptree::value_type &bodyValue, body) {
        if (bodyValue.first == "p") {
            std::string idAttr = bodyValue.second.get<std::string>("<xmlattr>.id", "");
            bool needNewSection = currentSection.title.empty() || !idAttr.empty();
            if (needNewSection && !currentSection.title.empty()) {
                sections.push_back(currentSection);
                currentSection = {};
                sectionNumber++;
            }
            std::string sectionId = "section" + std::to_string(sectionNumber);
            if (needNewSection) {
                currentSection.id = sectionId;
                currentSection.title = BEAUTY::getFlatPTreeContent(bodyValue.second);
            } else {
                std::set<std::string> thisClasses = allClassesOfTree(bodyValue.second);
                std::set<std::string> thisItalicClasses;

                set_intersection(
                    thisClasses.begin(), thisClasses.end(),
                    italicClasses.begin(), italicClasses.end(),
                    inserter(thisItalicClasses, thisItalicClasses.begin())
                );

                ArticleSectionParagraph paragraph;
                paragraph.content = BEAUTY::getFlatPTreeContent(bodyValue.second);
                paragraph.is_italic = !thisItalicClasses.empty();
                currentSection.paragraphs.push_back(paragraph);
            }
        } else {
            ArticleSectionParagraph paragraph;
            pt::ptree thisTree;
            thisTree.push_back(bodyValue);
            paragraph.tree = thisTree;
            currentSection.paragraphs.push_back(paragraph);
        }
    }
    if (!currentSection.title.empty()) {
        sections.push_back(currentSection);
    }

    document.sections = sections;
    return document;
}

}
