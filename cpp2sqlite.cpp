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

#include "sqlDatabase.hpp"
#include "beautify.hpp"
#include "atc.hpp"
#include "epha.hpp"
#include "gtin.hpp"
#include "peddose.hpp"
#include "report.hpp"

#include "ean13/functii.h"

// Fool boost parser so it doesn't create children for <sub> and <sup>
#define WORKAROUND_SUB_SUP
#define ESCAPED_SUB_L         "[[[[sub]]]]"  // "&lt;sub&gt;" would be altered by boost parser
#define ESCAPED_SUB_R         "[[[[/sub]]]]"
#define ESCAPED_SUP_L         "[[[[sup]]]]"
#define ESCAPED_SUP_R         "[[[[/sup]]]]"

#define WITH_PROGRESS_BAR
#define USE_BOOST_FOR_REPLACEMENTS // faster than with std::regex
//#define DEBUG_SHOW_RAW_XML_IN_DB_FILE

#define TITLES_STR_SEPARATOR    ";"

namespace po = boost::program_options;
namespace pt = boost::property_tree;

static std::string appName;
std::map<std::string, std::string> statsTitleStrSeparatorMap;

void on_version()
{
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

static
std::string getBarcodesFromGtins(const GTIN::oneFachinfoPackages &packages)
{
    std::string html;
    int i=0;
    for (auto gtin : packages.gtin) {
        
        if (i < packages.name.size()) // possibly redundant check
            html += "  <p class=\"spacing1\">" + packages.name[i++] + "</p>\n";
        
        std::string svg = EAN13::createSvg("", gtin);
        // TODO: onmouseup="addShoppingCart(this)"
        html += "<p class=\"barcode\">" + svg + "</p>\n";
    }

    return html;

}


// Modify <colgroup>, see HtmlUtils.java:525
// Add all the values first into a `sum` variable,
// then each value is multiplied by 100 and divided by `sum`
//
// Special cases to be tested:
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

static void cleanupXml(std::string &xml,
                       const std::string regnrs)
{
    // See also HtmlUtils.java:934
    std::regex r1(R"(<span[^>]*>)");
    xml = std::regex_replace(xml, r1, "");
    
    std::regex r2(R"(</span>)");
    xml = std::regex_replace(xml, r2, "");
    
#if 0
    std::regex r6a(R"(')");
    xml = std::regex_replace(xml, r6a, "&apos;"); // to prevent errors when inserting into sqlite table
#endif
    
    // The Java version seems to be using Jsoup and EscapeMode.xhtml
    // Don't convert &lt; &gt; &apos;
    boost::replace_all(xml, "&nbsp;",   " ");
    boost::replace_all(xml, "&ge;",     "≥");
    boost::replace_all(xml, "&le;",     "≤");
    boost::replace_all(xml, "&plusmn;", "±"); // used in rn 58868 table 6
    boost::replace_all(xml, "&agrave;", "à");
    boost::replace_all(xml, "&acirc;",  "â");
    boost::replace_all(xml, "&Acirc;",  "Â");
    boost::replace_all(xml, "&auml;",   "ä");
    boost::replace_all(xml, "&Auml;",   "Ä");
    boost::replace_all(xml, "&egrave;", "è");
    boost::replace_all(xml, "&Egrave;", "È");
    boost::replace_all(xml, "&eacute;", "é");
    boost::replace_all(xml, "&Eacute;", "É");
    boost::replace_all(xml, "&ecirc;",  "ê");
    boost::replace_all(xml, "&euml;",   "ë");
    boost::replace_all(xml, "&iuml;",   "ï");
    boost::replace_all(xml, "&icirc;",  "î");
    boost::replace_all(xml, "&ouml;",   "ö");
    boost::replace_all(xml, "&ocirc;",  "ô");
    boost::replace_all(xml, "&Ouml;",   "Ö");
    boost::replace_all(xml, "&Ograve;", "Ò");
    boost::replace_all(xml, "&uuml;",   "ü");
    boost::replace_all(xml, "&Uuml;",   "Ü");
    boost::replace_all(xml, "&middot;", "–"); // the true middot is "·"
    boost::replace_all(xml, "&bdquo;",  "„");
    boost::replace_all(xml, "&ldquo;",  "“");
    boost::replace_all(xml, "&rsquo;",  "’");
    boost::replace_all(xml, "&alpha;",  "α");
    boost::replace_all(xml, "&beta;",   "β");
    boost::replace_all(xml, "&gamma;",  "γ");
    boost::replace_all(xml, "&kappa;",  "κ");
    boost::replace_all(xml, "&micro;",  "µ");
    boost::replace_all(xml, "&mu;",     "μ");
    boost::replace_all(xml, "&phi;",    "φ");
    boost::replace_all(xml, "&Phi;",    "Φ");
    boost::replace_all(xml, "&tau;",    "τ");
    boost::replace_all(xml, "&frac12;", "½");
    boost::replace_all(xml, "&mdash;",  "—");
    boost::replace_all(xml, "&ndash;",  "–");
    boost::replace_all(xml, "&bull;",   "•"); // See rn 63182. Where is this in the Java code ?
    boost::replace_all(xml, "&reg;",    "®");
    boost::replace_all(xml, "&copy;",   "©");
    boost::replace_all(xml, "&trade;",  "™");
    boost::replace_all(xml, "&laquo;",  "«");
    boost::replace_all(xml, "&raquo;",  "»");
    boost::replace_all(xml, "&deg;",    "°");
    boost::replace_all(xml, "&sup1;",   "¹");
    boost::replace_all(xml, "&sup2;",   "²");
    boost::replace_all(xml, "&sup3;",   "³");
    boost::replace_all(xml, "&times;",  "×");
    boost::replace_all(xml, "&pi;",     "π");
    boost::replace_all(xml, "&szlig;",  "ß");
    boost::replace_all(xml, "&infin;",  "∞");
    boost::replace_all(xml, "&dagger;", "†");
    boost::replace_all(xml, "&Dagger;", "‡");
    boost::replace_all(xml, "&sect;",   "§");
    boost::replace_all(xml, "&spades;", "♠"); // rn 63285, table 2
    boost::replace_all(xml, "&THORN;",  "Þ");
    boost::replace_all(xml, "&Oslash;", "Ø");
    boost::replace_all(xml, "&para;",   "¶");
    boost::replace_all(xml, "&amp;",    "&");
    boost::replace_all(xml, "&frasl;",  "⁄"); // see rn 36083
    boost::replace_all(xml, "&curren;", "¤");
    boost::replace_all(xml, "&yen;",    "¥");
    boost::replace_all(xml, "&pound;",  "£");
    boost::replace_all(xml, "&ordf;",   "ª");
    boost::replace_all(xml, "&ccedil;", "ç");

    // Cleanup XML post-replacements (still pre-parsing)
    
    // For section titles.
    // Make the child XML tag content part of the parent.
    // Also, the Reg mark is already "sup"
    boost::replace_all(xml, "<sup class=\"s3\">®</sup>", "®");
    boost::replace_all(xml, "<sup class=\"s3\">® </sup>", "®");

#ifdef DEBUG_SUB_SUP
    std::string::size_type pos;
    
    std::vector<size_t> posSup;
    pos = xml.find("<sup");
    while (pos != std::string::npos) {
        posSup.push_back(pos);
        pos = xml.find("<sup", pos+1);
    }
    
    for (auto p : posSup) {
        std::clog
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", found \"" << xml.substr(p,28) << "\""
        << ", rn:" << regnrs
        << ", pos:" << p
        << std::endl;
    }
    
    std::vector<size_t> posSub;
    pos = xml.find("<sub");
    while (pos != std::string::npos) {
        posSub.push_back(pos);
        pos = xml.find("<sub", pos+1);
    }
    
    for (auto p : posSub) {
        std::clog
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", found \"" << xml.substr(p,28) << "\""
        << ", rn:" << regnrs
        << ", pos:" << p
        << std::endl;
    }
    
#endif // DEBUG_SUB_SUP
    
#ifdef WORKAROUND_SUB_SUP
    // Temporarily alter these XML (HTML) tags so that
    // the boost parser doesn't treat them as "children"
    std::regex r12(R"(<sup[^>]*>)");
    xml = std::regex_replace(xml, r12, ESCAPED_SUP_L);
    
    std::regex r13(R"(</sup>)");
    xml = std::regex_replace(xml, r13, ESCAPED_SUP_R);
    
    std::regex r10(R"(<sub[^>]*>)");
    xml = std::regex_replace(xml, r10, ESCAPED_SUB_L);
    
    std::regex r11(R"(</sub>)");
    xml = std::regex_replace(xml, r11, ESCAPED_SUB_R);
    
#ifdef DEBUG_SUB_SUP_TRACE
    posSup.clear();
    pos = xml.find(ESCAPED_SUP_L);
    while (pos != std::string::npos) {
        posSup.push_back(pos);
        pos = xml.find(ESCAPED_SUP_L, pos+1);
    }
    
    for (auto p : posSup) {
        std::clog
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", xml becomes \"" << xml.substr(p,28) << "\""
        << ", pos:" << p
        << std::endl;
    }
    
    posSub.clear();
    pos = xml.find(ESCAPED_SUB_L);
    while (pos != std::string::npos) {
        posSub.push_back(pos);
        pos = xml.find(ESCAPED_SUB_L, pos+1);
    }
    
    for (auto p : posSub) {
        std::clog
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", xml becomes \"" << xml.substr(p,28) << "\""
        << ", pos:" << p
        << std::endl;
    }
#endif // DEBUG_SUB_SUP
#endif // WORKAROUND_SUB_SUP
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
                    bool verbose = false)
{
#ifdef DEBUG_SHOW_RAW_XML_IN_DB_FILE
    html = xml;
    return;
#endif
    
    //std::clog << basename((char *)__FILE__) << ":" << __LINE__ << " " << regnrs << std::endl;

    cleanupXml(xml, regnrs);

    pt::ptree tree;
    std::stringstream ss;
    ss << xml;
    read_xml(ss, tree);
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
        std::clog << "XML TYPE 2, regnrs " << regnrs << std::endl;
#endif
        
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
            std::clog << sId << ", " << sTitle << ", " << sectionNumber << std::endl;
#endif

            // Next
            posDivFrom = xml.find(divFromText, posDivFrom + divFromText.length());
        }
        
        // Insert barcodes
        std::string htmlBarcodes = getBarcodesFromGtins(packages);
        std::string barcodeFromText("<div class=\"absTitle\">Packungen</div>");
        if (language == "fr")
            barcodeFromText = "<div class=\"absTitle\">Présentation</div>";
        std::string::size_type posBarcodeFrom = xml.find(barcodeFromText);
        std::string::size_type from = posBarcodeFrom + barcodeFromText.length();
        std::string::size_type posBarcodeTo = xml.find("</div>", from);
        xml.replace(from, posBarcodeTo - from, htmlBarcodes);
        
#ifdef WORKAROUND_SUB_SUP
        // Restore
        boost::replace_all(xml, ESCAPED_SUP_L, "<sup>");
        boost::replace_all(xml, ESCAPED_SUP_R, "</sup>");
        boost::replace_all(xml, ESCAPED_SUB_L, "<sub>");
        boost::replace_all(xml, ESCAPED_SUB_R, "</sub>");
#endif
        
        // This type of XML requires very little processing
        html += xml;

        goto doPedDose;
    }

    html += "  <div id=\"monographie\" name=\"" + regnrs + "\">\n\n";

    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("div")) {
  
            // The following call changes &lt; into <
            std::string tagContent = v.second.data();

            // Don't skip tags with empty content because all tables are like that
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
            // because they cause problems inserting the data into sqlite tables
#ifdef USE_BOOST_FOR_REPLACEMENTS
            // Modify the content, not the HTML tags
            boost::replace_all(tagContent, "<", "&lt;");
            boost::replace_all(tagContent, ">", "&gt;");
            boost::replace_all(tagContent, "'", "&apos;");
            
#ifdef WORKAROUND_SUB_SUP
            // Restore
            boost::replace_all(tagContent, ESCAPED_SUP_L, "<sup>");
            boost::replace_all(tagContent, ESCAPED_SUP_R, "</sup>");
            boost::replace_all(tagContent, ESCAPED_SUB_L, "<sub>");
            boost::replace_all(tagContent, ESCAPED_SUB_R, "</sub>");
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
                    boost::replace_all(tagContent, "â", "®");
                    boost::replace_all(tagContent, "&apos;", "'");
                    if (tagContent.find(TITLES_STR_SEPARATOR) != std::string::npos) {
                        statsTitleStrSeparatorMap.insert(std::make_pair(regnrs, tagContent));
#ifdef DEBUG
                        if (verbose)
                            std::clog
                            << basename((char *)__FILE__) << ":" << __LINE__
                            << ", Warning - rn " << regnrs
                            << ", replacing title_str separator in \"" << tagContent << "\""
                            << std::endl;
#endif
                        
                        // Replace section separator ";" with something else
                        // (not ',' because it's the decimal point separator on some locales)
                        boost::replace_all(tagContent, ";",  "·"); // &middot;
                    }

                    // HTML superscript tags are not supported in chapter list
                    std::string chapterName = tagContent;
                    boost::replace_all(chapterName, "<sup>", "");
                    boost::replace_all(chapterName, "</sup>", "");
                    sectionTitle.push_back(chapterName);
                    
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
                        html += getBarcodesFromGtins(packages);
                        section18Done = true;
                    }
                    
                    // Append 'section#' to a vector to be used in column "ids_str"
                    sectionId.push_back(section);

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
#ifdef DEBUG
                                if (verbose) {
                                    std::cerr
                                    << basename((char *)__FILE__) << ":" << __LINE__
                                    << ", regnrs: " << regnrs
                                    << ", section " << sectionNumber
                                    << ", <img> Warning " << e.what() << std::endl;
                                }
#endif
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
#if 0
                std::clog
                << basename((char *)__FILE__) << ":" << __LINE__
                << ", table" << v.second.data()
                << std::endl;
#endif

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
#ifdef WORKAROUND_SUB_SUP
                // Restore
                boost::replace_all(table, ESCAPED_SUB_L, "<sub>");
                boost::replace_all(table, ESCAPED_SUB_R, "</sub>");
                boost::replace_all(table, ESCAPED_SUP_L, "<sup>");
                boost::replace_all(table, ESCAPED_SUP_R, "</sup>");
#endif
                html += table + "\n";
            } // if table
        } // BOOST div
    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error " << e.what() << std::endl;
    }

doPedDose:
    // PedDose
    {
        std::string pedHtml = PED::getHtmlByAtc(atc);
        if (!pedHtml.empty()) {
            std::string sectionPedDose("section" + std::to_string(++sectionNumber));
            std::string sectionPedDoseName("Swisspeddose");
            
#ifdef DEBUG_PED_DOSE
            static int kk=0;
            if (kk++ < 20)
                std::cout
                << basename((char *)__FILE__) << ":" << __LINE__
                << ", " << atc
                << std::endl;
#endif
            
            html += "   <div class=\"paragraph\" id=\"" + sectionPedDose + "\">";
            html += "<div class=\"absTitle\">" + sectionPedDoseName + "</div>";
            html += pedHtml;
            html += "   </div>\n";

            // Append 'section#' to a vector to be used in column "ids_str"
            sectionId.push_back(sectionPedDose);
            sectionTitle.push_back(sectionPedDoseName);
        }
    }

    // section21
    {
        html += "   <div class=\"paragraph\" id=\"section" + std::to_string(++sectionNumber) + "\"></div>\n";

        std::time_t seconds = std::time(nullptr);
        std::string curtime = std::asctime(std::localtime( &seconds )); // TODO: avoid trailing \n
        std::string url("https://github.com/zdavatz/");
        url += appName;
        html += "<p class=\"footer\">Auto-generated by <a href=\"" + url + "\">" + appName + "</a> on " + curtime + "</p>";
    }
    
    if (hasXmlHeader)
        html += "\n  </div>";

    html += "\n </body>";
    html += "\n</html>";
}

int main(int argc, char **argv)
{
    //std::setlocale(LC_ALL, "en_US.utf8");
    
    appName = boost::filesystem::basename(argv[0]);

    std::string opt_inputDirectory;
    std::string opt_workDirectory;  // for downloads subdirectory
    std::string opt_language;
    bool flagXml = false;
    bool flagVerbose = false;
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
            std::cout << appName << " " << __DATE__ << " " << __TIME__ << std::endl;
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

    REP::init(opt_workDirectory + "/output/", opt_language, flagVerbose);
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

    PED::parseXML(opt_workDirectory + "/downloads/swisspeddosepublication.xml",
                  opt_language);
#ifdef DEBUG_PED_DOSE
    PED::showPedDoseByAtc("N02BA01");
    PED::showPedDoseByAtc("J05AB01");
    PED::showPedDoseByAtc("N02BE01"); // Acetalgin, RN 34186,62355,49493
#endif

    // Read swissmedic next, because aips might need to get missing ATC codes from it
    SWISSMEDIC::parseXLXS(opt_workDirectory + "/downloads/swissmedic_packages.xlsx");

    ATC::parseTXT(opt_inputDirectory + "/atc_codes_multi_lingual.txt", opt_language, flagVerbose);

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

    if (flagXml) {
        std::cerr << "Creating XML not yet implemented" << std::endl;
    }
    else {
        std::string dbFilename = opt_workDirectory + "/output/amiko_db_full_idx_" + opt_language + ".db";
        sqlite3 *db = AIPS::createDB(dbFilename);

        sqlite3_stmt *statement;
        AIPS::prepareStatement("amikodb", &statement,
                               "null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?");

        std::clog << std::endl << "Populating " << dbFilename << std::endl;
        unsigned int statsRnFoundRefdataCount = 0;
        unsigned int statsRnNotFoundRefdataCount = 0;
        unsigned int statsRnFoundSwissmedicCount = 0;
        unsigned int statsRnNotFoundSwissmedicCount = 0;
        unsigned int statsRnFoundBagCount = 0;
        unsigned int statsRnNotFoundBagCount = 0;
        std::vector<std::string> statsRegnrsNotFound;

#ifdef WITH_PROGRESS_BAR
        int ii=1;
        int n=list.size();
#endif
        for (AIPS::Medicine m : list) {
            
#ifdef WITH_PROGRESS_BAR
            // Show progress
            if ((ii++ % 60) == 0)
                std::cerr << "\r" << 100*ii/n << " % ";
#endif

            // See DispoParse.java:164 addArticleDB()
            // See SqlDatabase.java:347 addExpertDB()
            AIPS::bindText("amikodb", statement, 1, m.title);
            AIPS::bindText("amikodb", statement, 2, m.auth);
            AIPS::bindText("amikodb", statement, 3, m.atc);
            AIPS::bindText("amikodb", statement, 4, m.subst);
            AIPS::bindText("amikodb", statement, 5, m.regnrs);
            
            // For each regnr in the vector add the name(s) from refdata
            std::vector<std::string> regnrs;
            boost::algorithm::split(regnrs, m.regnrs, boost::is_any_of(", "), boost::token_compress_on);
            //std::cerr << basename((char *)__FILE__) << ":" << __LINE__  << "regnrs size: " << regnrs.size() << std::endl;

            // atc_class
            std::string atcClass = ATC::getClassByAtcColumn(m.atc);
            AIPS::bindText("amikodb", statement, 6, atcClass);

            // tindex_str
            std::string tindex = BAG::getTindex(regnrs[0]);
            if (tindex.empty())
                AIPS::bindText("amikodb", statement, 7, "");
            else
                AIPS::bindText("amikodb", statement, 7, tindex);

            // application_str
            {
            std::string application = SWISSMEDIC::getApplication(regnrs[0]);
            std::string appBag = BAG::getApplication(regnrs[0]);
            if (!appBag.empty())
                application += ";" + appBag;

            if (application.empty())
                AIPS::bindText("amikodb", statement, 8, "");
            else
                AIPS::bindText("amikodb", statement, 8, application);
            }
            
            // TODO: indications_str
            AIPS::bindText("amikodb", statement, 9, "");
            
            // TODO: customer_id
            AIPS::bindText("amikodb", statement, 10, "");  // "0"

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
                AIPS::bindText("amikodb", statement, 11, "");
            else
                AIPS::bindText("amikodb", statement, 11, packInfo);
#endif

            // TODO: add_info__str
            AIPS::bindText("amikodb", statement, 12, "");

            // content
            auto firstAtc = ATC::getFirstAtcInAtcColumn(m.atc);
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
                               flagVerbose);
                AIPS::bindText("amikodb", statement, 15, html);
            }
            
            // ids_str
            {
                std::string ids_str = boost::algorithm::join(sectionId, ",");
                AIPS::bindText("amikodb", statement, 13, ids_str);
            }

            // titles_str
            {
                std::string titles_str = boost::algorithm::join(sectionTitle, TITLES_STR_SEPARATOR);
                AIPS::bindText("amikodb", statement, 14, titles_str);
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
                    // In the Java db there are 2 commas or 3 if there is SL
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

                AIPS::bindText("amikodb", statement, 17, packages);
            }
            
            AIPS::runStatement("amikodb", statement);
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

        AIPS::destroyStatement(statement);

        int rc = sqlite3_close(db);
        if (rc != SQLITE_OK)
            std::cerr
            << basename((char *)__FILE__) << ":" << __LINE__
            << ", rc" << rc
            << std::endl;
    }

    REP::terminate();

    return EXIT_SUCCESS;
}
