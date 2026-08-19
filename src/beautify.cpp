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
#include <unordered_map>
#include <algorithm>
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
// HTML entities that occur in the Refdata documents and have to become real
// characters. No replacement produces a '&', so these substitutions cannot
// cascade into one another - which is what makes the single pass in
// cleanupForNonHtmlUsage() below equivalent to replacing them one at a time.
static const std::unordered_map<std::string, std::string> &entityTable()
{
    static const std::unordered_map<std::string, std::string> table = {
        {"&nbsp;",   " "},
        {"&ge;",     "≥"},
        {"&le;",     "≤"},
        {"&plusmn;", "±"}, // used in rn 58868 table 6
        {"&agrave;", "à"},
        {"&Agrave;", "À"},
        {"&acirc;",  "â"},
        {"&Acirc;",  "Â"},
        {"&auml;",   "ä"},
        {"&Auml;",   "Ä"},
        {"&egrave;", "è"},
        {"&Egrave;", "È"},
        {"&eacute;", "é"},
        {"&Eacute;", "É"},
        {"&ecirc;",  "ê"},
        {"&euml;",   "ë"},
        {"&iuml;",   "ï"},
        {"&icirc;",  "î"},
        {"&ouml;",   "ö"},
        {"&ocirc;",  "ô"},
        {"&Ouml;",   "Ö"},
        {"&Ograve;", "Ò"},
        {"&uuml;",   "ü"},
        {"&Uuml;",   "Ü"},
        {"&oelig;",  "œ"},
        {"&OElig;",  "Œ"},
        {"&middot;", "–"}, // the true middot is "·"
        {"&bdquo;",  "„"},
        {"&ldquo;",  "“"},
        {"&lsquo;",  "‘"},
        {"&rsquo;",  "’"},
        {"&alpha;",  "α"},
        {"&beta;",   "β"},
        {"&gamma;",  "γ"},
        {"&kappa;",  "κ"},
        {"&micro;",  "µ"},
        {"&mu;",     "μ"},
        {"&phi;",    "φ"},
        {"&Phi;",    "Φ"},
        {"&tau;",    "τ"},
        {"&frac12;", "½"},
        {"&minus;",  "−"},
        {"&mdash;",  "—"},
        {"&ndash;",  "–"},
        {"&bull;",   "•"}, // See rn 63182. Where is this in the Java code ?
        {"&reg;",    "®"},
        {"&copy;",   "©"},
        {"&trade;",  "™"},
        {"&laquo;",  "«"},
        {"&raquo;",  "»"},
        {"&deg;",    "°"},
        {"&sup1;",   "¹"},
        {"&sup2;",   "²"},
        {"&sup3;",   "³"},
        {"&times;",  "×"},
        {"&pi;",     "π"},
        {"&szlig;",  "ß"},
        {"&infin;",  "∞"},
        {"&dagger;", "†"},
        {"&Dagger;", "‡"},
        {"&sect;",   "§"},
        {"&spades;", "♠"}, // rn 63285, table 2
        {"&THORN;",  "Þ"},
        {"&Oslash;", "Ø"},
        {"&para;",   "¶"},

        {"&frasl;",  "⁄"}, // see rn 36083
        {"&curren;", "¤"},
        {"&yen;",    "¥"},
        {"&pound;",  "£"},
        {"&ordf;",   "ª"},
        {"&ccedil;", "ç"},

        {"&larr;",   "←"},
        {"&uarr;",   "↑"},
        {"&rarr;",   "→"},
        {"&darr;",   "↓"},
        {"&harr;",   "↔"},
    };
    return table;
}

// Length of the longest key in entityTable(), so that adding a longer entity
// above cannot silently stop matching.
static std::string::size_type maxEntityLength()
{
    static const std::string::size_type len = []{
        std::string::size_type m = 0;
        for (const auto &entity : entityTable())
            m = std::max(m, entity.first.size());
        return m;
    }();
    return len;
}

// This used to be 76 consecutive boost::replace_all() calls, i.e. 76 full
// scans (each reallocating on every hit) of an ~85 KB document, for every one
// of the ~4500 Fachinfos. One pass with a lookup on each "&...;" token gives
// byte-identical output at a fraction of the cost.
void cleanupForNonHtmlUsage(std::string &xml)
{
    if (xml.find('&') == std::string::npos)
        return;

    const auto &table = entityTable();
    const std::string::size_type maxLen = maxEntityLength();

    std::string out;
    out.reserve(xml.size());

    std::string::size_type i = 0;
    while (i < xml.size()) {
        if (xml[i] == '&') {
            // A key runs from '&' up to the first following ';', so at most one
            // table entry can match here, exactly as replace_all would have.
            std::string::size_type semi = xml.find(';', i + 1);
            if (semi != std::string::npos && semi - i + 1 <= maxLen) {
                auto it = table.find(xml.substr(i, semi - i + 1));
                if (it != table.end()) {
                    out += it->second;
                    i = semi + 1;
                    continue;
                }
            }
        }
        out += xml[i++];
    }

    xml.swap(out);
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
        bool isSpanEmpty = isElementEmpty(span);

        if (isSpanEmpty) {
            tree.erase("span");
        } else if (!hasAttr) {
            pt::ptree spanContent = getTextAndImagePTree(span);
            tree.erase("span");
            for (auto &child : spanContent) {
                tree.push_back(child);
            }
        }
    }
}

bool isElementEmpty(pt::ptree tree) {
    bool isDataEmpty = tree.data().empty();
    bool hasNoChildren = tree.size() == 0;
    bool allChildrenEmpty = hasNoChildren || std::all_of(tree.begin(), tree.end(), [](const pt::ptree::value_type &v) {
        return isElementEmpty(v.second);
    });

    return isDataEmpty && allChildrenEmpty;
}

std::string getFlatPTreeContent(pt::ptree tree) {
    std::string result = tree.data();
    BOOST_FOREACH(pt::ptree::value_type &v, tree) {
        if (v.first != "<xmlattr>") {
            result += getFlatPTreeContent(v.second);
        }
    }
    return result;
}

pt::ptree getTextAndImagePTree(pt::ptree tree) {
    pt::ptree result;
    BOOST_FOREACH(pt::ptree::value_type &v, tree) {
        if (v.first == "<xmltext>") {
            result.add_child("<xmltext>", v.second);
        } else if (v.first == "img") {
            result.add_child("img", v.second);
        } else if (v.first != "<xmlattr>") {
            pt::ptree child = getTextAndImagePTree(v.second);
            for (pt::ptree::value_type &child_v : child) {
                result.add_child(child_v.first, child_v.second);
            }
        }
    }
    return result;
}

}
