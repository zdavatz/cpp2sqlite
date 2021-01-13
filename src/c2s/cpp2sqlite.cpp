//
//  cpp2sqlite.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 15 Jan 2019
//

#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <map>
#include <exception>
#include <regex>
//#include <clocale>
#include <algorithm>
#include <ctime>

#include <sqlite3.h>
#include <libgen.h>     // for basename()

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
//#include <boost/program_options/errors.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "aips.hpp"
#include "refdata.hpp"
#include "swissmedic.hpp"
#include "bag.hpp"
#include "sappinfo.hpp"

#include "sqlDatabase.hpp"
#include "beautify.hpp"
#include "atc.hpp"
#include "epha.hpp"
#include "gtin.hpp"
#include "peddose.hpp"
#include "drugshortage.hpp"
#include "report.hpp"
#include "config.h"

#include "ean13/functii.h"

// Fool boost parser so it doesn't create children for "<sub>" and "<sup>" and "<br />"
#define WORKAROUND_SUB_SUP_BR
#define ESCAPED_SUB_L         "[[[[sub]]]]"  // "&lt;sub&gt;" would be altered by boost parser
#define ESCAPED_SUB_R         "[[[[/sub]]]]"
#define ESCAPED_SUP_L         "[[[[sup]]]]"
#define ESCAPED_SUP_R         "[[[[/sup]]]]"
#define ESCAPED_BR            "[[[[br]]]]"   // Issue #30, rn 66547, section20, French

#define WITH_PROGRESS_BAR
#define USE_BOOST_FOR_REPLACEMENTS // faster than with std::regex
//#define DEBUG_SHOW_RAW_XML_IN_DB_FILE

#define TITLES_STR_SEPARATOR    ";"

// Additional sections, not in the XML
// If you change these numbers also update smartinfo.py near line 74
// Note: AmiKo (macOS) expects
//      prefix "section" for numbers <= 100
//      prefix "Section" for numbers > 100
#define SECTION_NUMBER_PEDDOSE          9050
#define SECTION_NUMBER_FOOTER           9051
#define SECTION_NUMBER_SAPPINFO         9052
#define SECTION_NUMBER_SAPPINFO_P       9053
#define SECTION_NUMBER_SAPPINFO_BF      9054

namespace po = boost::program_options;
namespace pt = boost::property_tree;

constexpr std::string_view TABLE_NAME_AMIKO = "amikodb";
constexpr std::string_view TABLE_NAME_PRODUCT = "productdb";
constexpr std::string_view TABLE_NAME_ANDROID = "android_metadata";

static std::string appName;
std::map<std::string, std::string> statsTitleStrSeparatorMap;
static DB::Sql sqlDb;

void on_version()
{
    std::cout << appName << " " << PROJECT_VER
              << ", " << __DATE__ << " " << __TIME__ << std::endl;

    std::cout << "C++ " << __cplusplus << std::endl;
    std::cout << "SQLITE_VERSION: " << SQLITE_VERSION << std::endl;
    std::cout << "BOOST_VERSION: " << BOOST_LIB_VERSION << std::endl;
}

int countAipsPackagesInSwissmedic(AIPS::MedicineList &list)
{
    int count = 0;
    for (AIPS::Medicine m : list) {
        std::vector<std::string> regnrs;
        boost::algorithm::split(regnrs, m.regnrs, boost::is_any_of(", "), boost::token_compress_on);
        for (auto rn : regnrs) {
            count += SWISSMEDIC::countRowsWithRn(rn);
        }
    }
    return count;
}

int countBagGtinInSwissmedic(std::vector<std::string> &list)
{
    int count = 0;
    for (auto g : list) {
        if (SWISSMEDIC::findGtin(g))
            count++;
    }

    return count;
}

int countBagGtinInRefdata(std::vector<std::string> &list)
{
    int count = 0;
    for (auto g : list) {
        if (REFDATA::findGtin(g))
            count++;
    }

    return count;
}

static std::string getBarcodesFromGtins(
    const GTIN::oneFachinfoPackages &packages, 
    const std::string language,
    std::vector<std::string> &sectionId,
    std::vector<std::string> &sectionTitle
) {
    std::string html;
    int i=0;
    for (auto gtin : packages.gtin) {
        auto drugShortage = DRUGSHORTAGE::getEntryByGtin(std::stoll(gtin));
        bool hasDrugshortage = !drugShortage.empty();
        if (i < packages.name.size()) {// possibly redundant check
            html += "  <p class=\"spacing1\">" + packages.name[i++];
            if (hasDrugshortage) {
                html += "<span style='color:red'>●</span>";
            }
            html += "</p>\n";
        }

        std::string svg = EAN13::createSvg("", gtin);
        // TODO: onmouseup="addShoppingCart(this)"
        html += "<p class=\"barcode\">" + svg + "</p>\n";

        try {
            if (hasDrugshortage) {
                html += "<p>";
                std::string title = language == "de" ? "Drugshortage" : "Drugshortge";
                html += " <div class=\"absTitle\" id=\"section18-" + gtin + "\">\n" + title + "\n </div>\n";
                if (drugShortage.contains("status")) {
                    html += "Status: " + drugShortage["status"].get<std::string>() + "<br>\n";
                }
                if (drugShortage.contains("datumLieferfahigkeit")) {
                    html += "Geschaetztes Datum Lieferfaehigkeit: " + drugShortage["datumLieferfahigkeit"].get<std::string>() + "<br>\n";
                }
                if (drugShortage.contains("datumLetzteMutation")) {
                    html += "Datum Letzte Mutation: " + drugShortage["datumLetzteMutation"].get<std::string>() + "\n";
                }
                html += "</p>";
                sectionId.push_back("section18-" + gtin);
                sectionTitle.push_back(title);
            }
        } catch (...) {}
    }

    return html;

}


// Modify <colgroup>, see HtmlUtils.java:525
// Add all the values first into a `sum` variable,
// then each value is multiplied by 100 and divided by `sum`
//
// TODO: special cases to be tested:
//  rn 65553 has an empty table before section 1
//  rn 56885 has only one col
void modifyColgroup(pt::ptree &colgroup)
{
    float sum = 0.0;
    std::vector<float> oldValue;

    BOOST_FOREACH(pt::ptree::value_type &col, colgroup) {
        std::string style = col.second.get<std::string>("<xmlattr>.style");

        float val = 0.0;
        // Test string: "width:1.77222in;"
        std::regex rgx(R"(\d*\.\d*)");  // tested at https://regex101.com
        std::smatch match;
        if (std::regex_search(style, match, rgx)) {
            val = std::stof(match[0]);
        }

        oldValue.push_back(val);
        sum += val;
    }

    int index = 0;
    BOOST_FOREACH(pt::ptree::value_type &col, colgroup) {
        float newValue = 100.0 * oldValue[index++] / sum;

        std::ostringstream s;
        s << std::fixed << std::setprecision(6) << newValue;
        std::string newStyle = "width:" + s.str() + "%25;background-color: #EEEEEE; padding-right: 5px; padding-left: 5px";

        col.second.put("<xmlattr>.style", newStyle);
    }
}

void removeTagFromXml(std::string &xml, const std::string &tag)
{

}

// Here we modify the HTML contents and possibly children tags, not the parent tags
static void cleanupTitle(std::string &title)
{
    boost::replace_all(title, "&amp;", "&"); // rn 66547, section 20, French
    // HTML superscript tags are not supported in the chapter list
    boost::replace_all(title, "<sub>", "");
    boost::replace_all(title, "</sub>", "");
    boost::replace_all(title, "<sup>", "");
    boost::replace_all(title, "</sup>", "");
}

static void cleanupSection_1_Title(std::string &title)
{
    cleanupTitle(title);

    // All titles in XML "type 2" terminate with "<br />". Remove it
    size_t lastindex = title.rfind("<br />");
    if (lastindex != std::string::npos) {
        // Remove the "<br />" suffix
        title = title.substr(0, lastindex);

        // Some titles have another "<br />" in the middle
        // Leave it there for the HTML
    }
}

// Maybe it should be treated the same as section 1 (TBC)
static void cleanupSection_not1_Title(std::string &title)
{
    cleanupTitle(title);

    // rn 66547, section 20, French
    size_t lastindex = title.find("<br />");
    if (lastindex != std::string::npos) {
        // Keep only up to the first "<br />"
        title = title.substr(0, lastindex);
    }
}

static void cleanupSection_not1_Title(std::string &title,
                                      const std::string regnrs)
{
    cleanupSection_not1_Title(title);

    if (title.find(TITLES_STR_SEPARATOR) != std::string::npos) {
        statsTitleStrSeparatorMap.insert(std::make_pair(regnrs, title));

        // Replace section separator ";" with something else
        // (not ',' because it's the decimal point separator on some locales)
        boost::replace_all(title, TITLES_STR_SEPARATOR,  "·"); // &middot;
    }
}

// see RealExpertInfo.java:1065
void getHtmlFromXml(std::string &xml,
                    std::string &html,
                    std::string regnrs,
                    std::string ownerCompany,
                    const GTIN::oneFachinfoPackages &packages, // for barcodes
                    std::vector<std::string> &sectionId,
                    std::vector<std::string> &sectionTitle,
                    const std::string atc,
                    const std::string language,
                    bool verbose,
                    bool skipSappinfo)
{
#ifdef DEBUG_SHOW_RAW_XML_IN_DB_FILE
    html = xml;
    return;
#endif

#ifdef DEBUG
    if (atc.empty())
        std::clog << basename((char *)__FILE__) << ":" << __LINE__
        << ", Empty ATC for rn:" << regnrs
        << std::endl;
#endif

    //std::clog << basename((char *)__FILE__) << ":" << __LINE__ << " " << regnrs << std::endl;

    BEAUTY::cleanupXml(xml, regnrs);  // and escape some children tags

    pt::ptree tree;
    std::stringstream ss;
    ss << xml;
    pt::read_xml(ss, tree);
    int sectionNumber = 0;
    unsigned int statsParCount=0;
    bool section1Done = false;
    bool section18Done = false;

    html = "<html>\n";
    html += " <head></head>\n";
    html += " <body>\n";

    bool hasXmlHeader = boost::starts_with(xml, "<?xml version");
    if (!hasXmlHeader) {

#ifdef DEBUG
        //std::clog << "XML TYPE 2, regnrs " << regnrs << std::endl;
#endif

        // Title
        // All we have to do for XML "type 2" is add an attribute
        // id="section1" end clean up the "<br />"

        // Extract the title
        std::regex rgx(R"(<div class=\"MonTitle\">(.*)</div>)");    // tested at https://regex101.com
        std::smatch match;
        if (std::regex_search(xml, match, rgx)) {
            std::string title = match[match.size() - 1];

            // Note: the title from "MonTitle" is used in two places in AmiKo,
            // in the middle pane (HTML), and on the right pane (chapter name)

            // Restore children tags
            boost::replace_all(title, ESCAPED_SUB_L, "<sub>");
            boost::replace_all(title, ESCAPED_SUB_R, "</sub>");
            boost::replace_all(title, ESCAPED_SUP_L, "<sup>");
            boost::replace_all(title, ESCAPED_SUP_R, "</sup>");
            boost::replace_all(title, ESCAPED_BR,    "<br />");

            // HTML
            std::string titleDiv = "   <div class=\"MonTitle\" id=\"section1\">\n";
            titleDiv += title + "\n";
            titleDiv += "   </div>\n";
            xml = std::regex_replace(xml, rgx, titleDiv);

            // Note: "ownerCompany" is a separate "<div>" between section 1 and section 2
            // It's already there for XML type 2

            // Chapter name
            sectionId.push_back("Section1");
            cleanupSection_1_Title(title);
            // Some titles have another "<br />" in the middle
            // Leave it there for the HTML above
            // Remove it for the section 1 chapter name
            // For other section numbers see 'cleanupSection_not1_Title()'
            boost::replace_first(title, "<br />", " ");
            sectionTitle.push_back(title);
        } // if MonTitle

        // Extract chapter list
        const std::string sectIdText("id=\"Section");
        const std::string sectTitleText("<div class=\"absTitle\">");
        const std::string divFromText("<div class=\"paragraph\" id=\"Section");

        std::string::size_type posDivFrom = xml.find(divFromText);
        while (posDivFrom != std::string::npos) {

            // Append 'section#' to a vector to be used in column "ids_str"
            std::string::size_type posIdFrom = xml.find("Section", posDivFrom);
            std::string::size_type posIdTo   = xml.find("\">", posIdFrom);
            std::string sId = xml.substr(posIdFrom, posIdTo - posIdFrom);
            sectionNumber = std::stoi(sId.substr(7)); // "Section"
            sectionId.push_back(sId);

            // Append the section name to a vector to be used in column "titles_str"
            std::string::size_type posTitleFrom = xml.find(sectTitleText, posDivFrom + divFromText.length());
            std::string::size_type posTitleTo   = xml.find("</div>", posTitleFrom);
            std::string::size_type from = posTitleFrom + sectTitleText.length();
            std::string sTitle = xml.substr(from, posTitleTo - from);
            sectionTitle.push_back(sTitle);

#ifdef DEBUG
            //std::clog << sId << ", " << sTitle << ", " << sectionNumber << std::endl;
#endif

            // Next
            posDivFrom = xml.find(divFromText, posDivFrom + divFromText.length());
        }

        // Insert barcodes
        std::string htmlBarcodes = getBarcodesFromGtins(packages, language, sectionId, sectionTitle);
        std::string barcodeFromText("<div class=\"absTitle\">Packungen</div>");
        if (language == "fr")
            barcodeFromText = "<div class=\"absTitle\">Présentation</div>";
        std::string::size_type posBarcodeFrom = xml.find(barcodeFromText);
        std::string::size_type from = posBarcodeFrom + barcodeFromText.length();
        std::string::size_type posBarcodeTo = xml.find("</div>", from);
        xml.replace(from, posBarcodeTo - from, "\n" + htmlBarcodes);

#ifdef WORKAROUND_SUB_SUP_BR
        // Restore children
        boost::replace_all(xml, ESCAPED_SUP_L, "<sup>");
        boost::replace_all(xml, ESCAPED_SUP_R, "</sup>");
        boost::replace_all(xml, ESCAPED_SUB_L, "<sub>");
        boost::replace_all(xml, ESCAPED_SUB_R, "</sub>");
        boost::replace_all(xml, ESCAPED_BR,    "<br />");
#endif

        // Remove the closing "</div>".
        // We'll put it back later after adding the last two extra sections: "peddose" and "auto-generated"
        size_t lastindex = xml.rfind("</div>"); // find_last_of() would return the pos at the end of the suffix
        xml = xml.substr(0, lastindex);

        // This type of XML requires very little processing
        html += xml;

        goto doExtraSections;
    }

    html += "  <div id=\"monographie\" name=\"" + regnrs + "\">\n\n";

    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("div")) {

            // The following call changes "&lt;" into "<"
            // "&amp;" into "&"
            std::string tagContent = v.second.data();

            // Don't skip XML tags with empty content because all tables are like that
//            if (tagContent.empty()) {
//                std::clog << basename((char *)__FILE__) << ":" << __LINE__ << std::endl;
//                continue;
//            }

#ifdef DEBUG_SUB_SUP_TRACE
            std::string::size_type pos = tagContent.find(ESCAPED_SUP_L);
            if (pos != std::string::npos) {
                std::clog
                << basename((char *)__FILE__) << ":" << __LINE__
                << ", found \"" << tagContent.substr(pos,28) << "\""
                << ", pos:" << pos
                << std::endl;
            }

            pos = tagContent.find("<sup>");
            if (pos != std::string::npos) {
                std::clog
                << basename((char *)__FILE__) << ":" << __LINE__
                << ", found \"" << tagContent.substr(pos,28) << "\""
                << ", pos:" << pos
                << std::endl;
            }
#endif

            // Undo then undesired replacements done by boost xml_parser
#ifdef USE_BOOST_FOR_REPLACEMENTS
            // Modify the content, not the HTML tags
            if (!tagContent.empty()) {
                boost::replace_all(tagContent, "<", "&lt;");
                boost::replace_all(tagContent, ">", "&gt;");
                boost::replace_all(tagContent, "'", "&apos;");
                boost::replace_all(tagContent, " & ", " &amp; "); // rn 66547, section 20, French
            }

#ifdef WORKAROUND_SUB_SUP_BR
            // Restore children
            boost::replace_all(tagContent, ESCAPED_SUP_L, "<sup>");
            boost::replace_all(tagContent, ESCAPED_SUP_R, "</sup>");
            boost::replace_all(tagContent, ESCAPED_SUB_L, "<sub>");
            boost::replace_all(tagContent, ESCAPED_SUB_R, "</sub>");
            boost::replace_all(tagContent, ESCAPED_BR,    "<br />");
#endif
#else
            std::regex r1("<");
            tagContent = std::regex_replace(tagContent, r1, "&lt;");

            std::regex r2(">");
            tagContent = std::regex_replace(tagContent, r2, "&gt;");

            //std::regex r3("'");
            //tagContent = std::regex_replace(tagContent, r3, "&apos;");
#endif

            if (v.first == "p")
            {
                ++statsParCount;

                bool isSection = true;
                std::string section;
                try {
                    section = v.second.get<std::string>("<xmlattr>.id");

#if 0
                    if (section.substr(1,6) != "ection") // section or Section
                        std::cout
                        << basename((char *)__FILE__) << ":" << __LINE__
                        << ", Warning - rn " << regnrs
                        << ", unexpected attribute id=\"" << section << "\""
                        << std::endl;
#endif

                    sectionNumber = std::stoi(section.substr(7));  // from position 7 to end

                    // Append the section name to a vector to be used in column "titles_str"
                    // Make sure it doesn't already contain the separator ";"
                    boost::replace_all(tagContent, "Ò", "®"); // see HtmlUtils.java:636
                    if (language == "de")
                        boost::replace_all(tagContent, "â", "®");

                    boost::replace_all(tagContent, "&apos;", "'");

                    std::string chapterName = tagContent;
                    cleanupSection_not1_Title(chapterName, regnrs);

                    sectionTitle.push_back(chapterName);
                    // Append 'section#' to a vector to be used in column "ids_str"
                    sectionId.push_back(section);

                    std::string divClass;
                    if (sectionNumber == 1) {
                        divClass = "MonTitle";
                    }
                    else {
                        divClass = "paragraph";
                        tagContent = " <div class=\"absTitle\">\n " + tagContent + "\n </div>\n";
                    }

                    if (sectionNumber > 1)
                        html += "   </div>\n"; // terminate previous section before starting a new one

                    html += "   <div class=\"" + divClass + "\" id=\"" + section + "\">\n";
                    html += tagContent + "\n";
                    //html += "   </div>\n";  // don't terminate the div as yet
#if 0
                    std::clog
                    << basename((char *)__FILE__) << ":" << __LINE__
                    << ", p count " << statsParCount
                    << ", attr id: <" << section << ">"
                    << ", nr: <" << sectionNumber << ">"
                    << std::endl;
#endif
                    if (sectionNumber == 1) {
                        // ownerCompany is a separate div between section 1 and section 2
                        html += "   </div>\n";  // terminate previous section
                        html += "   <div class=\"ownerCompany\">\n";
                        html += "    <div style=\"text-align: right;\">\n   ";
                        html += ownerCompany + "\n";
                        html += "    </div>\n";
                        //html += "   </div>\n";    // don't terminate the div as yet
                        section1Done = true;
                    }

                    // see RealExpertInfo.java:1562
                    // see BarCode.java:77
                    if (sectionNumber == 18) {
                        html += getBarcodesFromGtins(packages, language, sectionId, sectionTitle);
                        section18Done = true;
                    }

                    continue;
                }
                catch (std::exception &e) {
                    isSection = false;
                    //std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error " << e.what() << std::endl;
                }  // try section

                // Skip before section 1
                if (!section1Done)
                    continue;

                // Skip all the remaining before section 2
                if ((sectionNumber < 2) && (section1Done))
                    continue;

                //if (sectionNumber == 13) images can be anywhere
                {
                    bool imgFound = false;
                    BOOST_FOREACH(pt::ptree::value_type &v2, v.second) {
                        if (v2.first == "img") {
                            imgFound = true;

                            std::string img = "<img";

                            std::string src = v2.second.get<std::string>("<xmlattr>.src");
                            img += " Src=\"" + src + "\"";

                            std::string style = v2.second.get<std::string>("<xmlattr>.style");
                            if (!style.empty())
                                img += " Style=\"" + style + "\"";

                            std::string alt;
                            try {
                                alt = v2.second.get<std::string>("<xmlattr>.alt");
                                if (!alt.empty())
                                    img += " Alt=\"" + alt + "\"";
                            }
                            catch (std::exception &e) {
                                AIPS::addStatsMissingAlt(regnrs,sectionNumber);
                            }

                            img += " />";

                            html += "  <p class=\"spacing1\">" + img + "</p>\n";
                        }
                    } // BOOST

                    if (imgFound)
                        continue;  // already added this <p> to html
                }

                // Skip all the remaining in section 18
                if ((sectionNumber == 18) && (section18Done))
                    continue;

                // See HtmlUtils.java:472
                bool needItalicSpan = true;
                boost::algorithm::trim(tagContent); // sometimes it ends with ". "

                if (tagContent.empty()) // for example in rn 51704
                    continue; // TBC

                if (boost::ends_with(tagContent, ".") ||
                    boost::ends_with(tagContent, ",") ||
                    boost::ends_with(tagContent, ":") ||
                    boost::contains(tagContent, "ATC-Code") ||
                    boost::contains(tagContent, "Code ATC"))
                {
                    needItalicSpan = false;
                }

                // See HtmlUtils.java:602
#if 0
                std::regex r("^[–|·|-|•]");
                tagContent = std::regex_replace(tagContent, r, "– "); // FIXME: it becomes "– \200\223 "
#else
                if (boost::starts_with(tagContent, "–")) {      // en dash
                    boost::replace_first(tagContent, "–", "– ");
                    needItalicSpan = false;
                }
                else if (boost::starts_with(tagContent, "·")) {
                    boost::replace_first(tagContent, "·", "– ");
                    needItalicSpan = false;
                }
                else if (boost::starts_with(tagContent, "-")) { // hyphen
                    boost::replace_first(tagContent, "-", "– ");
                    needItalicSpan = false;
                }
                else if (boost::starts_with(tagContent, "•")) {
                    boost::replace_first(tagContent, "•", "– ");
                    needItalicSpan = false;
                }
//                else if (boost::starts_with(tagContent, "*")) {  // TBC see table footnote for rn 51704
//                    needItalicSpan = false;
//                }
#endif

                if (needItalicSpan)
                    tagContent = "<span style=\"font-style:italic;\">" + tagContent + "</span>";

                html += "  <p class=\"spacing1\">" + tagContent + "</p>\n";

            } // if p
            else if (v.first == "table") {
                // Normalize column widths to a percentage value
                pt::ptree &colgroup = v.second.get_child("colgroup");
                modifyColgroup(colgroup);

                // Purpose: add the table to the html "as is"
                // Method: create a new property tree, string based, not file based
                //         and add the whole table object as the only child
                pt::ptree tree;
                std::stringstream ss;
                tree.add_child("table", v.second);
                pt::write_xml(ss, tree);

                std::string table = ss.str();

                // Clean up the "serialized" string
                boost::replace_all(table, "<?xml version=\"1.0\" encoding=\"utf-8\"?>", "");
#ifdef WORKAROUND_SUB_SUP_BR
                // Restore children
                boost::replace_all(table, ESCAPED_SUB_L, "<sub>");
                boost::replace_all(table, ESCAPED_SUB_R, "</sub>");
                boost::replace_all(table, ESCAPED_SUP_L, "<sup>");
                boost::replace_all(table, ESCAPED_SUP_R, "</sup>");
                boost::replace_all(table, ESCAPED_BR,    "<br />");
#endif
                html += table + "\n";
            } // if table
        } // BOOST div
    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error " << e.what() << std::endl;
    }

#pragma mark - extra sections

doExtraSections:
    // Add a section that was not in the XML contents
    // PedDose
    if (!atc.empty() && !PED::isRegnrsInBlacklist(regnrs))
    {
        std::string pedHtml = PED::getHtmlByAtc(atc);
        if (!pedHtml.empty()) {
            std::string sectionPedDose("Section" + std::to_string(SECTION_NUMBER_PEDDOSE));
            std::string sectionPedDoseName("Swisspeddose");

            if (hasXmlHeader)
                html += "\n  </div>"; // terminate previous section before starting a new one

            html += "   <div class=\"paragraph\" id=\"" + sectionPedDose + "\">\n";
            html += "<div class=\"absTitle\">" + sectionPedDoseName + "</div>";
            html += pedHtml;
            html += "   </div>\n";

            // Append 'section#' to a vector to be used in column "ids_str"
            sectionId.push_back(sectionPedDose);
            sectionTitle.push_back(sectionPedDoseName);
        }
    }

    // Add another section that was not in the XML contents
    // Sappinfo
    if (!atc.empty() && !skipSappinfo)
    {
        std::string htmlPregnancy;
        std::string htmlBreastfeed;
        SAPP::getHtmlByAtc(atc, htmlPregnancy, htmlBreastfeed);

        if (!htmlPregnancy.empty()) {
            const std::string sectionSappInfo1("Section" + std::to_string(SECTION_NUMBER_SAPPINFO_P));
            std::string sectionSappInfoName1("SAPP: Schwangere");
            if (language == "fr")
                sectionSappInfoName1 = "ASPP: F. enceintes";


            if (hasXmlHeader && atc.empty())
                html += "\n  </div>"; // terminate previous section before starting a new one

            html += "   <div class=\"paragraph\" id=\"" + sectionSappInfo1 + "\">\n";
            html += "<div class=\"absTitle\">" + sectionSappInfoName1 + "</div>";
            html += htmlPregnancy;
            html += "   </div>\n";

            // Append 'section#' to a vector to be used in column "ids_str"
            sectionId.push_back(sectionSappInfo1);
            sectionTitle.push_back(sectionSappInfoName1);
        }

        if (!htmlBreastfeed.empty()) {
            const std::string sectionSappInfo2("Section" + std::to_string(SECTION_NUMBER_SAPPINFO_BF));
            std::string sectionSappInfoName2("SAPP: Stillende");
            if (language == "fr")
                sectionSappInfoName2 = "ASPP: F. allaitantes";

            if (hasXmlHeader && atc.empty())
                html += "\n  </div>"; // terminate previous section before starting a new one

            html += "   <div class=\"paragraph\" id=\"" + sectionSappInfo2 + "\">\n";
            html += "<div class=\"absTitle\">" + sectionSappInfoName2 + "</div>";
            html += htmlBreastfeed;
            html += "   </div>\n";

            // Append 'section#' to a vector to be used in column "ids_str"
            sectionId.push_back(sectionSappInfo2);
            sectionTitle.push_back(sectionSappInfoName2);
        }
    }

    // Add a section that was not in the XML contents
    // Note that this section id and name don't get added to the chapter name list
    // Footer
    {
        html += "   <div class=\"paragraph\" id=\"Section" + std::to_string(SECTION_NUMBER_FOOTER) + "\"></div>\n";

        std::time_t seconds = std::time(nullptr);
        std::string curtime = std::asctime(std::localtime( &seconds )); // TODO: avoid trailing \n
        std::string url("https://github.com/zdavatz/");
        url += appName;
        html += "<p class=\"footer\">Auto-generated by <a href=\"" + url + "\">" + appName + "</a> on " + curtime + "</p>";
    }

    html += "\n  </div>";
    html += "\n </body>";
    html += "\n</html>";
}

// See SqlDatabase.java:65
void openDB(const std::string &filename)
{
    std::clog << std::endl << "Create DB: " << filename << std::endl;

    sqlDb.openDB(filename);

    sqlDb.createTable(TABLE_NAME_AMIKO, "_id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, auth TEXT, atc TEXT, substances TEXT, regnrs TEXT, atc_class TEXT, tindex_str TEXT, application_str TEXT, indications_str TEXT, customer_id INTEGER, pack_info_str TEXT, add_info_str TEXT, ids_str TEXT, titles_str TEXT, content TEXT, style_str TEXT, packages TEXT");
    sqlDb.createIndex(TABLE_NAME_AMIKO, "idx_", {"title", "auth", "atc", "substances", "regnrs", "atc_class"});
    sqlDb.prepareStatement(TABLE_NAME_AMIKO,
                           "null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?");

    sqlDb.createTable(TABLE_NAME_PRODUCT, "_id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, author TEXT, eancodes TEXT, pack_info_str TEXT, packages TEXT");
    sqlDb.createIndex(TABLE_NAME_PRODUCT, "idx_prod_", {"title", "author", "eancodes"});

    sqlDb.createTable(TABLE_NAME_ANDROID, "locale TEXT default 'en_US'");
    sqlDb.insertInto(TABLE_NAME_ANDROID, "locale", "'en_US'");

    //createTable("sqlite_sequence", "");  // created automatically
}

void closeDB()
{
    sqlDb.destroyStatement();
    sqlDb.closeDB();
}

#pragma mark -

int main(int argc, char **argv)
{
    //std::setlocale(LC_ALL, "en_US.utf8");

    appName = boost::filesystem::basename(argv[0]);

    std::string opt_inputDirectory;
    std::string opt_workDirectory;  // for downloads subdirectory
    std::string opt_language;
    bool flagXml = false;
    bool flagVerbose = false;
    bool flagNoSappinfo = false;
    //bool flagPinfo = false;
    std::string type("fi"); // Fachinfo
    std::string opt_aplha;
    std::string opt_regnr;
    std::string opt_owner;

    // See file Aips2Sqlite.java, function commandLineParse(), line 71, line 179
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "print this message")
        ("version,v", "print the version information and exit")
        ("verbose", "be extra verbose") // Show errors and logs
        ("without-sappinfo", "don't include sappinfo section")
//        ("nodown", "no download, parse only")
        ("lang", po::value<std::string>( &opt_language )->default_value("de"), "use given language (de/fr)")
//        ("alpha", po::value<std::string>( &opt_aplha ), "only include titles which start with arg value")  // Med title
//        ("regnr", po::value<std::string>( &opt_regnr ), "only include medications which start with arg value") // Med regnr
//        ("owner", po::value<std::string>( &opt_owner ), "only include medications owned by arg value") // Med owner
//        ("pseudo", "adds pseudo expert infos to db") // Pseudo fi
//        ("inter", "adds drug interactions to db")
//        ("pinfo", "generate patient info htmls") // Generate pi
//        ("xml", "generate xml file")
//        ("gln", "generate csv file with Swiss gln codes") // Shopping cart
//        ("shop", "generate encrypted files for shopping cart")
//        ("onlyshop", "skip generation of sqlite database")
//        ("zurrose", "generate zur Rose full article database or stock/like files (fulldb/atcdb/quick)") // Zur Rose DB
//        ("desitin", "generate encrypted files for Desitin")
//        ("onlydesitin", "skip generation of sqlite database") // Only Desitin DB
//        ("takeda", po::value<float>(), "generate sap/gln matching file")
//        ("dailydrugcosts", "calculates the daily drug costs")
//        ("smsequence", "generates swissmedic sequence csv")
//        ("packageparse", "extract dosage information from package name")
//        ("zip", "generate zipped big files (sqlite or xml)")
//        ("reports", "generates various reports")
//        ("indications", "generates indications section keywords report")
//        ("plain", "does not update the package section")
//        ("test", "starts in test mode")
//        ("stats", po::value<float>(), "generates statistics for given user")
        ("inDir", po::value<std::string>( &opt_inputDirectory )->required(), "input directory") //  without trailing '/'
        ("workDir", po::value<std::string>( &opt_workDirectory ), "parent of 'downloads' and 'output' directories, default as parent of inDir ")
        ;

    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm); // populate vm

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }

        if (vm.count("version")) {
            on_version();
            return EXIT_SUCCESS;
        }

        po::notify(vm); // raise any errors encountered
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (po::error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Exception of unknown type!" << std::endl;
        return EXIT_FAILURE;
    }

    if (vm.count("verbose")) {
        flagVerbose = true;
    }

    if (vm.count("without-sappinfo")) {
        flagNoSappinfo = true;
    }

    if (vm.count("xml")) {
        flagXml = true;
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << " flagXml: " << flagXml << std::endl;
    }

    if (vm.count("pinfo")) {
        //flagPinfo = true;
        type = "pi";
        //std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << " flagPinfo: " << flagPinfo << std::endl;
    }

    if (!vm.count("workDir")) {
        opt_workDirectory = opt_inputDirectory + "/..";
    }

    std::string reportFilename("amiko_report_" + opt_language + ".html");
    std::string language = opt_language;
    boost::to_upper(language);
    //ofs2 << "<title>" << title << " Report " << language << "</title>";
    std::string reportTitle("AmiKo Report " + language);
    REP::init(opt_workDirectory + "/output/", reportFilename, reportTitle, flagVerbose);
    REP::html_start_ul();
    for (int i=0; i<argc; i++)
        REP::html_li(argv[i]);

    REP::html_end_ul();

    // TODO: create index with links to expected h1 titles
    REP::html_h1("File Analysis");

#if 0
    // Read epha first, because aips needs to get missing ATC codes from it
    std::string jsonFilename = "/epha_products_" + opt_language + "_json.json";
    EPHA::parseJSON(opt_workDirectory + "/downloads" + jsonFilename, flagVerbose);
#endif

    PED::parseXML(opt_workDirectory + "/downloads/swisspeddosepublication_v3.xml",
                  opt_language);
    PED::parseBlacklistTXT(opt_inputDirectory + "/swisspeddose_blacklist.txt");
#ifdef DEBUG_PED_DOSE
    PED::showPedDoseByAtc("N02BA01");
    PED::showPedDoseByAtc("J05AB01");
    PED::showPedDoseByAtc("N02BE01"); // Acetalgin, RN 34186,62355,49493
#endif

    // Read swissmedic next, because aips might need to get missing ATC codes from it
    SWISSMEDIC::parseXLXS(opt_workDirectory + "/downloads/swissmedic_packages.xlsx");

    ATC::parseTXT(opt_inputDirectory + "/atc_codes_multi_lingual.txt", opt_language, flagVerbose);

    DRUGSHORTAGE::parseJSON(opt_workDirectory + "/downloads/drugshortage.json");

    AIPS::MedicineList &list = AIPS::parseXML(opt_workDirectory + "/downloads/aips.xml",
                                              opt_language,
                                              type,
                                              flagVerbose);

    REP::html_p("Swissmedic has " + std::to_string(countAipsPackagesInSwissmedic(list)) + " matching packages");

    REFDATA::parseXML(opt_workDirectory + "/downloads/refdata_pharma.xml", opt_language);

    BAG::parseXML(opt_workDirectory + "/downloads/bag_preparations.xml", opt_language, flagVerbose);
    {
        std::vector<std::string> bagList = BAG::getGtinList();
        REP::html_h4("Cross-reference");
        REP::html_start_ul();
        REP::html_li(std::to_string(countBagGtinInSwissmedic(bagList)) + " GTIN are also in swissmedic");
        REP::html_li(std::to_string(countBagGtinInRefdata(bagList)) + " GTIN are also in refdata");
        REP::html_end_ul();
    }

    if (!flagNoSappinfo)
        SAPP::parseXLXS(opt_inputDirectory, "/sappinfo.xlsx", opt_language);

    if (flagXml) {
        std::cerr << "Creating XML not yet implemented" << std::endl;
    }
    else {
        std::string dbFilename = opt_workDirectory + "/output/amiko_db_full_idx_" + opt_language + ".db";
        openDB(dbFilename);

        unsigned int statsRnFoundRefdataCount = 0;
        unsigned int statsRnNotFoundRefdataCount = 0;
        unsigned int statsRnFoundSwissmedicCount = 0;
        unsigned int statsRnNotFoundSwissmedicCount = 0;
        unsigned int statsRnFoundBagCount = 0;
        unsigned int statsRnNotFoundBagCount = 0;
        std::vector<std::string> statsRegnrsNotFound;

#ifdef WITH_PROGRESS_BAR
        int ii = 1;
        int n = list.size();
#endif
        for (AIPS::Medicine m : list) {

#ifdef WITH_PROGRESS_BAR
            // Show progress
            if ((ii++ % 60) == 0)
                std::cerr << "\r" << 100*ii/n << " % ";
#endif

            // For each regnr in the vector add the name(s) from refdata
            std::vector<std::string> regnrs;
            boost::algorithm::split(regnrs, m.regnrs, boost::is_any_of(", "), boost::token_compress_on);
            //std::cerr << basename((char *)__FILE__) << ":" << __LINE__  << ", regnrs size: " << regnrs.size() << std::endl;

            if (regnrs[0] == "00000")
                continue;

            // See DispoParse.java:164 addArticleDB()
            // See SqlDatabase.java:347 addExpertDB()
            sqlDb.bindText(1, m.title);
            sqlDb.bindText(2, m.auth);
            sqlDb.bindText(3, m.atc);
            sqlDb.bindText(4, m.subst);
            sqlDb.bindText(5, m.regnrs);

            // atc_class
            std::string atcClass = ATC::getClassByAtcColumn(m.atc);
            sqlDb.bindText(6, atcClass);

            // tindex_str
            std::string tindex = BAG::getTindex(regnrs[0]);
            if (tindex.empty())
                sqlDb.bindText(7, "");
            else
                sqlDb.bindText(7, tindex);

            // application_str
            {
            std::string application = SWISSMEDIC::getApplication(regnrs[0]);
            std::string appBag = BAG::getApplicationByRN(regnrs[0]);
            if (!appBag.empty())
                application += ";" + appBag;

            if (application.empty())
                sqlDb.bindText(8, "");
            else
                sqlDb.bindText(8, application);
            }

            // TODO: indications_str
            sqlDb.bindText(9, "");

            // TODO: customer_id
            sqlDb.bindText(10, "");  // "0"

#if 1
            // pack_info_str
            GTIN::oneFachinfoPackages packages;
            std::set<std::string> gtinUsedSet; // To ensure we don't have duplicates, and for stats
            for (auto rn : regnrs) {
                //std::cerr << basename((char *)__FILE__) << ":" << __LINE__  << " rn: " << rn << std::endl;

                // Search in refdata
                int nAdd = REFDATA::getNames(rn, gtinUsedSet, packages);
                if (nAdd == 0)
                    statsRnNotFoundRefdataCount++;
                else
                    statsRnFoundRefdataCount++;

                // Search in swissmedic
                nAdd = SWISSMEDIC::getAdditionalNames(rn, gtinUsedSet, packages, opt_language);
                if (nAdd == 0)
                    statsRnNotFoundSwissmedicCount++;
                else
                    statsRnFoundSwissmedicCount++;

                // Search in bag
                nAdd = BAG::getAdditionalNames(rn, gtinUsedSet, packages);
                if (nAdd == 0)
                    statsRnNotFoundBagCount++;
                else
                    statsRnFoundBagCount++;

                if (gtinUsedSet.empty())
                    statsRegnrsNotFound.push_back(rn);
            } // for

            BEAUTY::sort(packages);

            // Create a single multi-line string from the vector
            std::string packInfo = boost::algorithm::join(packages.name, "\n");

            if (packInfo.empty())
                sqlDb.bindText(11, "");
            else
                sqlDb.bindText(11, packInfo);
#endif

            // TODO: add_info__str
            sqlDb.bindText(12, "");

            // content
            auto firstAtc = ATC::getFirstAtcInAtcColumn(m.atc);
            if (firstAtc.empty()) {
#ifdef DEBUG
                std::clog << basename((char *)__FILE__) << ":" << __LINE__
                << ", title: <" << m.title << ">"
                << ", atc: <" << m.atc << ">"     // nicht vergeben
                << std::endl;
#endif
            }

            std::vector<std::string> sectionId;    // HTML section IDs
            std::vector<std::string> sectionTitle; // HTML section titles
            {
                std::string html;
                getHtmlFromXml(m.content, html, m.regnrs, m.auth,
                               packages,        // for barcodes
                               sectionId,       // for ids_str
                               sectionTitle,    // for titles_str
                               firstAtc,        // for pedDose
                               opt_language,    // for barcode section
                               flagVerbose,
                               flagNoSappinfo);
                sqlDb.bindText(15, html);
            }

            // ids_str
            {
                std::string ids_str = boost::algorithm::join(sectionId, ",");
                sqlDb.bindText(13, ids_str);
            }

            // titles_str
            {
                std::string titles_str = boost::algorithm::join(sectionTitle, TITLES_STR_SEPARATOR);
                sqlDb.bindText(14, titles_str);
            }

            // TODO: style_str

            // packages
            {
                // The line order must be the same as pack_info_str
                std::vector<std::string>::iterator itGtin = packages.gtin.begin();
                std::vector<std::string> lines;
                for (auto name : packages.name) {

                    SWISSMEDIC::dosageUnits du = SWISSMEDIC::getByGtin(*itGtin);
                    BAG::packageFields pf = BAG::getPackageFieldsByGtin(*itGtin);

                    // Field 0
                    // TODO: temporarily use the first part of the name
                    std::string::size_type len = name.find(",");
                    std::string oneLine = name.substr(0, len);  // pos, len

                    oneLine += "|";

                    // Field 1
                    oneLine += du.dosage;
                    oneLine += "|";

                    // Field 2
                    oneLine += du.units;
                    oneLine += "|";

                    // Field 3
                    if (!pf.efp.empty())
                        oneLine += "CHF " + pf.efp;

                    oneLine += "|";

                    // Field 4
                    if (!pf.pp.empty())
                        oneLine += "CHF " + pf.pp;

                    // Fields 5,6,7
                    // no FAP FEP VAT
                    oneLine += "||||";

                    // Field 8
                    // In the Java DB there are 2 commas or 3 if there is SL
                    oneLine += boost::algorithm::join(pf.flags, ",");
                    oneLine += "|";

                    // Field 9
                    oneLine += *itGtin;
                    oneLine += "|";

                    // Field 10
                    oneLine += REFDATA::getPharByGtin(*itGtin);

                    // Fields 11 and 12
                    oneLine += "|255|0";    // visibility flag, free samples

                    lines.push_back(oneLine);
                    itGtin++;
                }

                // Create a single multi-line string from the vector
                std::string packages = boost::algorithm::join(lines, "\n");

                sqlDb.bindText(17, packages);
            }

            sqlDb.runStatement(TABLE_NAME_AMIKO);
        } // for

#ifdef WITH_PROGRESS_BAR
        std::cerr << "\r100 %" << std::endl;
#endif
        REP::html_h1("Usage");

        REP::html_h2("aips REGNRS (found/not found)");
        REP::html_start_ul();
        REP::html_li("in refdata: " + std::to_string(statsRnFoundRefdataCount) + "/" + std::to_string(statsRnNotFoundRefdataCount) + " (" + std::to_string(statsRnFoundRefdataCount + statsRnNotFoundRefdataCount) + ")");
        REP::html_li("in swissmedic: " + std::to_string(statsRnFoundSwissmedicCount) + "/" + std::to_string(statsRnNotFoundSwissmedicCount) + " (" + std::to_string(statsRnFoundSwissmedicCount + statsRnNotFoundSwissmedicCount) + ")");
        REP::html_li("in bag: " + std::to_string(statsRnFoundBagCount) + "/" + std::to_string(statsRnNotFoundBagCount) + " (" + std::to_string(statsRnFoundBagCount + statsRnNotFoundBagCount) + ")");
        REP::html_li("not found anywhere: " + std::to_string(statsRegnrsNotFound.size()));
        REP::html_end_ul();
        if (statsRegnrsNotFound.size() > 0)
            REP::html_div(boost::algorithm::join(statsRegnrsNotFound, ", "));

        if (statsTitleStrSeparatorMap.size() > 0) {
            REP::html_h3("XML");
            REP::html_p("title_str separator '" + std::string(TITLES_STR_SEPARATOR) + "' was replaced for rgnrs");
            REP::html_start_ul();
            for (auto s : statsTitleStrSeparatorMap)
                REP::html_li(s.first + " \"" + s.second + "\"");

            REP::html_end_ul();
        }

        AIPS::printUsageStats();
        REFDATA::printUsageStats();
        SWISSMEDIC::printUsageStats();
        BAG::printUsageStats();
        ATC::printUsageStats();
        PED::printUsageStats();
        if (!flagNoSappinfo) {
            SAPP::printUsageStats();
        }

        closeDB();

    } // if flagXml

    REP::terminate();

    return EXIT_SUCCESS;
}
