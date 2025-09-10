//
//  beautify.cpp
//  cpp2sqlite, pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 29 Jan 2019
//

#include <string>
#include <sstream>
#include <regex>
#include <libgen.h>     // for basename()

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include "beautify.hpp"

namespace BEAUTY
{

void beautifyName(std::string &name)
{
    char separator = ' ';

    // Uppercase the first word (up to the first space)
    std::string::size_type pos1 = name.find(separator);
    auto token1 = name.substr(0, pos1); // pos, len
    token1 = boost::to_upper_copy<std::string>(token1);

    // Lowercase the rest
    auto token2 = name.substr(pos1+1); // pos, len
    token2 = boost::to_lower_copy<std::string>(token2);

    name = token1 + separator + token2;
}

// Sort package infos and gtins maintaining their pairing
// The sorting rule is
//      first packages with price
//      then packages without price
void sort(GTIN::oneFachinfoPackages &packages)
{
    if (packages.name.size() < 2)
        return;     // nothing to sort

#if 1 // Possibly redundant check now
    // For a couple of packages: 26395 SOLCOSERYL, and 37397 VENTOLIN
    // we have more pack info lines than gtins, because there are some
    // doubles pack info lines
    if (packages.name.size() != packages.gtin.size()) {
        std::cerr << std::endl
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", ERROR - pack info lines: " << packages.name.size()
        << ", gtin used: " << packages.gtin.size()
        << std::endl;

        for (auto line : packages.name)
            std::clog << "\tinfo " << line << std::endl;

        for (auto g : packages.gtin)
            std::clog << "\tgtin " << g << std::endl;

        return; // impossible to sort
    }
#endif

    // Start sorting, first by presence of price
    std::regex r(", EFP ");

    // Analyze
    std::vector<std::string> linesWithPrice;
    std::vector<std::string> linesWithoutPrice;

    std::vector<std::string> gtinsWithPrice;
    std::vector<std::string> gtinsWithoutPrice;
    std::vector<std::string>::iterator itGtin;

    itGtin = packages.gtin.begin();
    for (auto line : packages.name)
    {
        if (std::regex_search(line, r)) {
            linesWithPrice.push_back(line);
            gtinsWithPrice.push_back(*itGtin);
        }
        else {
            linesWithoutPrice.push_back(line);
            gtinsWithoutPrice.push_back(*itGtin);
        }

        itGtin++;
    }

    // TODO: sort by galenic form each of the two vectors

    // Prepare the results
    packages.name.clear();
    packages.gtin.clear();

    //std::string s;

    itGtin = gtinsWithPrice.begin();
    for (auto l : linesWithPrice) {
        packages.name.push_back(l);
        packages.gtin.push_back(*itGtin++);
    }

    itGtin = gtinsWithoutPrice.begin();
    for (auto l : linesWithoutPrice) {
        packages.name.push_back(l);
        packages.gtin.push_back(*itGtin++);
    }
}

//void sortByGalenicForm(std::vector<std::string> &group)
//{
//
//}

// TODO: instead of calling it for XML (tags and contents)
// call it only for tables (tags and content), chapter titles (contents), and xml "type 2" (tags and contents)
//
// The Java version seems to be using Jsoup and EscapeMode.xhtml
// Don't convert &lt; &gt; &apos;
void cleanupForNonHtmlUsage(std::string &xml)
{
    boost::replace_all(xml, "&nbsp;",   " ");
    boost::replace_all(xml, "&ge;",     "≥");
    boost::replace_all(xml, "&le;",     "≤");
    boost::replace_all(xml, "&plusmn;", "±"); // used in rn 58868 table 6
    boost::replace_all(xml, "&agrave;", "à");
    boost::replace_all(xml, "&Agrave;", "À");
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
    boost::replace_all(xml, "&oelig;",  "œ");
    boost::replace_all(xml, "&OElig;",  "Œ");
    boost::replace_all(xml, "&middot;", "–"); // the true middot is "·"
    boost::replace_all(xml, "&bdquo;",  "„");
    boost::replace_all(xml, "&ldquo;",  "“");
    boost::replace_all(xml, "&lsquo;",  "‘");
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
    boost::replace_all(xml, "&minus;",  "−");
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

    boost::replace_all(xml, "&frasl;",  "⁄"); // see rn 36083
    boost::replace_all(xml, "&curren;", "¤");
    boost::replace_all(xml, "&yen;",    "¥");
    boost::replace_all(xml, "&pound;",  "£");
    boost::replace_all(xml, "&ordf;",   "ª");
    boost::replace_all(xml, "&ccedil;", "ç");

    boost::replace_all(xml, "&larr;", "←");
    boost::replace_all(xml, "&uarr;", "↑");
    boost::replace_all(xml, "&rarr;", "→");
    boost::replace_all(xml, "&darr;", "↓");
    boost::replace_all(xml, "&harr;", "↔");
}

// Cleanup and also escape some children tags
void cleanupXml(std::string &xml,
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

    cleanupForNonHtmlUsage(xml); // unescapeContentForNonHtmlUsage

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

#ifdef WORKAROUND_SUB_SUP_BR
    // Temporarily alter these XML (HTML) tags so that
    // the boost parser doesn't treat them as "children"
    std::regex r10(R"(<sup[^>]*>)");
    xml = std::regex_replace(xml, r10, ESCAPED_SUP_L);

    std::regex r11(R"(</sup>)");
    xml = std::regex_replace(xml, r11, ESCAPED_SUP_R);

    std::regex r12(R"(<sub[^>]*>)");
    xml = std::regex_replace(xml, r12, ESCAPED_SUB_L);

    std::regex r13(R"(</sub>)");
    xml = std::regex_replace(xml, r13, ESCAPED_SUB_R);

    std::regex r14(R"(<br />)");
    xml = std::regex_replace(xml, r14, ESCAPED_BR);

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
#endif // WORKAROUND_SUB_SUP_BR
}

std::string escapeHtml(std::string str) {
    std::string result = str;
    boost::replace_all(result, "<", "&lt;");
    boost::replace_all(result, ">", "&gt;");
    return result;
}

// This function remove useless <span attr=xxx></span> and replace <span>1234</span> with just 1234
void cleanUpSpan(pt::ptree &tree) {
    BOOST_FOREACH(pt::ptree::value_type &v, tree) {
        cleanUpSpan(v.second);
    }

    pt::ptree empty_ptree;

    int childrenCount = 0;
    bool hasSpan = false;
    BOOST_FOREACH(pt::ptree::value_type &v, tree) {
        if (v.first != "<xmlattr>") {
            childrenCount++;
        }
        if (v.first == "span") {
            hasSpan = true;
        }
    }

    if (childrenCount == 1 && hasSpan) {
        pt::ptree span = tree.get_child("span");
        bool hasAttr = span.get_child("<xmlattr>", empty_ptree).size() > 0;
        std::string spanContent = getFlatPTreeContent(span);

        if (spanContent.empty()) {
            tree.erase("span");
        } else if (!hasAttr) {
            tree.erase("span");
            tree.put("<xmltext>", spanContent);
        }
    }
}

std::string getFlatPTreeContent(pt::ptree tree) {
    std::string result = tree.data();
    BOOST_FOREACH(pt::ptree::value_type &v, tree) {
        if (v.first != "<xmlattr>") {
            result += getFlatPTreeContent(v.second);
        }
    }
    boost::algorithm::trim(result);
    return result;
}

}
