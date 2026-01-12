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
#include <boost/algorithm/string/regex.hpp>
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
#include "batchrecalls.hpp"
#include "dhpchpc.hpp"
#include "swissreg.hpp"
#include "report.hpp"
#include "config.h"

#include "bagFHIR.hpp"

#include "ean13/functii.h"

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
#define SECTION_NUMBER_BATCH_RECALL     9055
#define SECTION_NUMBER_DHPC_HPC         9056
#define SECTION_NUMBER_SWISSREG         9057

namespace po = boost::program_options;
namespace pt = boost::property_tree;

constexpr std::string_view TABLE_NAME_AMIKO = "amikodb";
constexpr std::string_view TABLE_NAME_PRODUCT = "productdb";
constexpr std::string_view TABLE_NAME_ANDROID = "android_metadata";

static std::string appName;
std::map<std::string, std::string> statsTitleStrSeparatorMap;
std::vector<std::string> statsInvalidContentHTML;
std::vector<std::string> statsATCNotFound;
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
    int sectionNumber = 0;
    for (auto gtin : packages.gtin) {
        auto drugShortage = DRUGSHORTAGE::getEntryByGtin(std::stoll(gtin));
        bool hasDrugshortage = drugShortage.gtin != 0;
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
                std::string title = language == "de" ? "Drugshortage" : "Drugshortage";
                html += " <div class=\"absTitle\" id=\"Section18" + std::to_string(sectionNumber) + "\">\n" + title + "\n </div>\n";
                // margin for more space at the bottom https://github.com/zdavatz/cpp2sqlite/issues/167
                html += "<p class=\"spacing1\" style=\"margin-bottom:1em\">";
                if (drugShortage.status != "") {
                    if (language == "fr") {
                        html += "Statut: " + drugShortage.status + "<br/>\n";
                    } else {
                        html += "Status: " + drugShortage.status + "<br/>\n";
                    }
                }
                if (drugShortage.estimatedDateOfDelivery != "") {
                    if (language == "fr") {
                        html += "Date prévue d'accouchement: " + drugShortage.estimatedDateOfDelivery + "<br/>\n";
                    } else {
                        html += "Geschaetztes Datum Lieferfaehigkeit: " + drugShortage.estimatedDateOfDelivery + "<br/>\n";
                    }
                }
                if (drugShortage.dateLastMutation != "") {
                    if (language == "fr") {
                        html += "Date du dernier changement: " + drugShortage.dateLastMutation + "\n";
                    } else {
                        html += "Datum Letzte Mutation: " + drugShortage.dateLastMutation + "\n";
                    }
                }
                html += "</p>";
                sectionId.push_back("Section18" + std::to_string(sectionNumber));
                sectionTitle.push_back(title);
            }
        } catch (...) {}
        sectionNumber++;
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
void getHtmlFromXml(std::string &path,
                    std::string &html,
                    std::string regnrs,
                    std::string ownerCompany,
                    const GTIN::oneFachinfoPackages &packages, // for barcodes
                    std::vector<std::string> &sectionIds,
                    std::vector<std::string> &sectionTitles,
                    const std::string atc,
                    const std::string language,
                    bool verbose,
                    bool skipSappinfo)
{

#ifdef DEBUG
    if (atc.empty())
        std::clog << basename((char *)__FILE__) << ":" << __LINE__
        << ", Empty ATC for rn:" << regnrs
        << std::endl;
#endif

    std::string xml;

    REFDATA::ArticleDocument document = REFDATA::getArticleDocument(path);

    for (REFDATA::ArticleSection section : document.sections) {
        sectionIds.push_back(section.id);
        sectionTitles.push_back(section.title);
    }

    pt::ptree tree;
    tree.put("html.body.div.<xmlattr>.id", "monographie");
    tree.put("html.body.div.<xmlattr>.name", regnrs);
    for (REFDATA::ArticleSection section : document.sections) {
        if (section.id == "section1") {
            pt::ptree titleDiv = pt::ptree(section.title);
            titleDiv.put("<xmlattr>.class", "MonTitle");
            titleDiv.put("<xmlattr>.id", section.id);
            tree.add_child("html.body.div.div", titleDiv);

            pt::ptree ownerCompanyDiv;
            ownerCompanyDiv.put("<xmlattr>.class", "ownerCompany");
            ownerCompanyDiv.put("div.<xmlattr>.style", "text-align: right");
            ownerCompanyDiv.put("div.<xmltext>", ownerCompany);
            tree.add_child("html.body.div.div", ownerCompanyDiv);
            continue;
        }

        pt::ptree thisDiv;
        thisDiv.put("<xmlattr>.class", "paragraph");
        thisDiv.put("<xmlattr>.id", section.id);
        thisDiv.put("div.<xmlattr>.class", "absTitle");
        thisDiv.put("div.<xmltext>", section.title);

        if (section.title == "Packungen" || (language == "fr" && section.title == "Présentation")) {
            std::string htmlBarcodes = getBarcodesFromGtins(packages, language, sectionIds, sectionTitles);
            std::stringstream barcodeSS;
            barcodeSS << htmlBarcodes;
            pt::ptree barcodeTree;
            try {
                pt::read_xml(barcodeSS, barcodeTree, pt::xml_parser::no_concat_text);
            } catch (std::exception &e) {
                std::cerr << "Error reading barcode html: " << htmlBarcodes << std::endl;
                std::cerr << "Line: " << __LINE__ << " Error " << e.what() << std::endl;
            }
            BOOST_FOREACH(pt::ptree::value_type &v, barcodeTree) {
                thisDiv.push_back(pt::ptree::value_type(v.first, v.second));
            }
        } else {
            for (REFDATA::ArticleSectionParagraph paragraph : section.paragraphs) {
                if (!paragraph.content.empty()) {
                    pt::ptree paragraphP = pt::ptree(paragraph.content);
                    paragraphP.put("<xmlattr>.class", "spacing1");
                    if (paragraph.is_italic) {
                        paragraphP.put("<xmlattr>.style", "font-style: italic");
                    }
                    thisDiv.push_back(pt::ptree::value_type("p", paragraphP));
                } else {
                    BOOST_FOREACH(pt::ptree::value_type &pv, paragraph.tree) {
                        thisDiv.push_back(pt::ptree::value_type(pv.first, pv.second));
                    }
                }
            }
        }
        tree.get_child("html.body.div").push_back(pt::ptree::value_type("div", thisDiv));
    }

    if (!atc.empty() && !PED::isRegnrsInBlacklist(regnrs))
    {
        std::string pedHtml = PED::getHtmlByAtc(atc);
        if (!pedHtml.empty()) {
            std::string sectionPedDose("Section" + std::to_string(SECTION_NUMBER_PEDDOSE));
            std::string sectionPedDoseName("Swisspeddose");

            std::string extraHtml;
            extraHtml += "   <div class=\"paragraph\" id=\"" + sectionPedDose + "\">\n";
            extraHtml += "<div class=\"absTitle\">" + sectionPedDoseName + "</div>";
            extraHtml += pedHtml;
            extraHtml += "   </div>\n";

            try {
                std::stringstream pedSS;
                pedSS << extraHtml;
                pt::ptree extraHtmlTree;
                pt::read_xml(pedSS, extraHtmlTree, pt::xml_parser::no_concat_text);

                // Append 'section#' to a vector to be used in column "ids_str"
                sectionIds.push_back(sectionPedDose);
                sectionTitles.push_back(sectionPedDoseName);

                tree.add_child("html.body.div.div", extraHtmlTree.get_child("div"));
            } catch (std::exception &e) {
                std::clog << "Error adding ped xml: " << extraHtml << std::endl;
                throw e;
            }
        }
    }

    // Add another section that was not in the XML contents
    // Sappinfo
    if (!atc.empty() && !skipSappinfo)
    {
        std::string htmlPregnancy;
        std::string htmlBreastfeed;
        SAPP::getHtmlByAtc(atc, htmlPregnancy, htmlBreastfeed);
        std::string extraHtml;

        if (!htmlPregnancy.empty()) {
            const std::string sectionSappInfo1("Section" + std::to_string(SECTION_NUMBER_SAPPINFO_P));
            std::string sectionSappInfoName1("SAPP: Schwangere");
            if (language == "fr")
                sectionSappInfoName1 = "ASPP: F. enceintes";

            extraHtml += "   <div class=\"paragraph\" id=\"" + sectionSappInfo1 + "\">\n";
            extraHtml += "<div class=\"absTitle\">" + sectionSappInfoName1 + "</div>";
            extraHtml += htmlPregnancy;
            extraHtml += "   </div>\n";

            // Append 'section#' to a vector to be used in column "ids_str"
            sectionIds.push_back(sectionSappInfo1);
            sectionTitles.push_back(sectionSappInfoName1);
        }

        if (!htmlBreastfeed.empty()) {
            const std::string sectionSappInfo2("Section" + std::to_string(SECTION_NUMBER_SAPPINFO_BF));
            std::string sectionSappInfoName2("SAPP: Stillende");
            if (language == "fr")
                sectionSappInfoName2 = "ASPP: F. allaitantes";

            extraHtml += "   <div class=\"paragraph\" id=\"" + sectionSappInfo2 + "\">\n";
            extraHtml += "<div class=\"absTitle\">" + sectionSappInfoName2 + "</div>";
            extraHtml += htmlBreastfeed;
            extraHtml += "   </div>\n";

            // Append 'section#' to a vector to be used in column "ids_str"
            sectionIds.push_back(sectionSappInfo2);
            sectionTitles.push_back(sectionSappInfoName2);
        }
        if (!extraHtml.empty()) {
            try {
                std::stringstream ss;
                ss << extraHtml;
                pt::ptree extraHtmlTree;
                pt::read_xml(ss, extraHtmlTree, pt::xml_parser::no_concat_text);

                BOOST_FOREACH(pt::ptree::value_type &et, extraHtmlTree) {
                    tree.get_child("html.body.div").push_back(pt::ptree::value_type(et.first, et.second));
                }
            } catch (std::exception &e) {
                std::clog << "Error adding SAPPINFO xml: " << extraHtml << std::endl;
                throw e;
            }
        }
    }

    {
        std::string extraHtml;
        std::vector<std::string> regnrsList;
        boost::algorithm::split(regnrsList, regnrs, boost::is_any_of(", "), boost::token_compress_on);
        bool addedSectionTitle = false;
        for (auto regnrs : regnrsList) {
            auto recalls = BATCHRECALLS::getRecallsByRegnrs(regnrs);
            for (auto recall : recalls) {
                if (!addedSectionTitle) {
                    const std::string sectionBatchRecall("Section" + std::to_string(SECTION_NUMBER_BATCH_RECALL));
                    std::string sectionBatchRecallName("Chargenrückrufe");
                    if (language == "fr") {
                        sectionBatchRecallName = "Retraits de lots";
                    }
                    extraHtml += "   <div class=\"paragraph\" id=\"" + sectionBatchRecall + "\">\n";
                    extraHtml += "<div class=\"absTitle\">" + sectionBatchRecallName + "</div>";
                    addedSectionTitle = true;
                    sectionIds.push_back(sectionBatchRecall);
                    sectionTitles.push_back(sectionBatchRecallName);
                }
                extraHtml += "<p class=\"spacing1\">";
                if (recall.title.length()) {
                    if (language == "fr") {
                        extraHtml += "Titre: "+ recall.title + "<br/>\n";
                    } else {
                        extraHtml += "Titel: "+ recall.title + "<br/>\n";
                    }
                }
                if (recall.date.length()) {
                    if (language == "fr") {
                        extraHtml += "Date: "+ recall.date + "<br/>\n";
                    } else {
                        extraHtml += "Datum: "+ recall.date + "<br/>\n";
                    }
                }
                if (recall.regnrs.length()) {
                    if (language == "fr") {
                        extraHtml += "No d'autorisation: " + recall.regnrs + "<br/>\n";
                    } else {
                        extraHtml += "Zulassungsnummer: "+ recall.regnrs + "<br/>\n";
                    }
                }
                if (recall.description.length()) {
                    if (language == "fr") {
                        extraHtml += "Texte: "+ recall.description + "<br/>\n";
                    } else {
                        extraHtml += "Text: "+ recall.description + "<br/>\n";
                    }
                }
                for (auto extra : recall.extras) {
                    extraHtml += extra.first + ": "+ extra.second + "<br/>\n";
                }
                if (recall.pdfLink.length()) {
                    std::string link = recall.pdfLink;
                    boost::replace_all(link, "'", "&apos;");
                    extraHtml += "<a href='" + link + "'>PDF Link</a>\n";
                }
                extraHtml += "</p>";
            }
        }
        if (addedSectionTitle) {
            extraHtml += "</div>";
        }
        if (!extraHtml.empty()) {
            try {
                std::stringstream ss;
                ss << extraHtml;
                pt::ptree extraHtmlTree;
                pt::read_xml(ss, extraHtmlTree, pt::xml_parser::no_concat_text);

                tree.add_child("html.body.div.div", extraHtmlTree.get_child("div"));
            } catch (std::exception &e) {
                std::clog << "Error in Chargenrückrufe, xml: " << extraHtml;
                throw e;
            }
        }
    }

    {
        std::string extraHtml;
        std::vector<std::string> regnrsList;
        boost::algorithm::split(regnrsList, regnrs, boost::is_any_of(", "), boost::token_compress_on);
        bool addedSectionTitle = false;
        for (auto regnrs : regnrsList) {
            auto news = DHPCHPC::getNewsByRegnrs(regnrs);
            for (auto news : news) {
                if (!addedSectionTitle) {
                    const std::string sectionDHPCHPC("Section" + std::to_string(SECTION_NUMBER_DHPC_HPC));
                    std::string sectionDHPCHPCName("DHPC/HPC");
                    extraHtml += "   <div class=\"paragraph\" id=\"" + sectionDHPCHPC + "\">\n";
                    extraHtml += "<div class=\"absTitle\">" + sectionDHPCHPCName + "</div>";
                    addedSectionTitle = true;
                    sectionIds.push_back(sectionDHPCHPC);
                    sectionTitles.push_back(sectionDHPCHPCName);
                }
                extraHtml += "<p class=\"spacing1\">";
                if (news.title.length()) {
                    if (language == "fr") {
                        extraHtml += "Titre: "+ news.title + "<br/>\n";
                    } else {
                        extraHtml += "Titel: "+ news.title + "<br/>\n";
                    }
                }
                if (news.date.length()) {
                    if (language == "fr") {
                        extraHtml += "Date: "+ news.date + "<br/>\n";
                    } else {
                        extraHtml += "Datum: "+ news.date + "<br/>\n";
                    }
                }
                if (news.description.length()) {
                    std::string description = news.description;
                    boost::replace_all(description, "<", "&lt;");
                    if (language == "fr") {
                        extraHtml += "Texte: "+ description + "<br/>\n";
                    } else {
                        extraHtml += "Text: "+ description + "<br/>\n";
                    }
                }
                if (news.regnrs.length()) {
                    if (language == "fr") {
                        extraHtml += "No d'autorisation: " + news.regnrs + "<br/>\n";
                    } else {
                        extraHtml += "Zulassungsnummer: "+ news.regnrs + "<br/>\n";
                    }
                }
                for (auto extra : news.extras) {
                    extraHtml += extra.first + ": " + extra.second + "<br/>\n";
                }
                if (news.pdfLink.length()) {
                    extraHtml += "<a href='" + news.pdfLink + "'>PDF Link</a>\n";
                }
                extraHtml += "</p>";
            }
        }
        if (addedSectionTitle) {
            extraHtml += "</div>";
        }
        if (!extraHtml.empty()) {
            try {
                std::stringstream ss;
                ss << extraHtml;
                pt::ptree extraHtmlTree;
                pt::read_xml(ss, extraHtmlTree, pt::xml_parser::no_concat_text);

                tree.add_child("html.body.div.div", extraHtmlTree.get_child("div"));
            } catch (std::exception &e) {
                std::clog << "xml2: " << extraHtml;
                throw e;
            }
        }
    }

    {
        std::vector<std::string> regnrsList;
        boost::algorithm::split(regnrsList, regnrs, boost::is_any_of(", "), boost::token_compress_on);
        std::set<std::string> addedCertificateIds;
        pt::ptree extraHtmlTree;
        std::string sectionId = "Section" + std::to_string(SECTION_NUMBER_SWISSREG);
        std::string sectionName = "Swissreg";
        extraHtmlTree.put("<xmlattr>.class", "paragraph");
        extraHtmlTree.put("<xmlattr>.id", sectionId);

        extraHtmlTree.put("div.<xmlattr>.class", "absTitle");
        extraHtmlTree.put("div.<xmltext>", sectionName);

        for (auto regnrs : regnrsList) {
            for (auto cert : SWISSREG::getCertsByRegnr(regnrs)) {
                if (cert.certificateId.empty()) continue;
                if (addedCertificateIds.find(cert.certificateId) != addedCertificateIds.end()) continue;
                addedCertificateIds.insert(cert.certificateId);

                pt::ptree certLink;
                certLink.put("<xmlattr>.class", "spacing1");
                certLink.put("<xmltext>", language == "de" ? "ESZ/PESZ-Nummer:" : "CCP/CCP pédiatrique:");
                certLink.put("a.<xmltext>", cert.certificateNumber);
                std::string certId = cert.certificateId;
                boost::replace_all(certId, "urn:ige:schutztitel:esz:", "");
                certLink.put("a.<xmlattr>.href", "https://www.swissreg.ch/database-client/register/detail/certificate/" + certId + "?lang=" + language);
                certLink.put("a.<xmlattr>.target", "_blank");
                extraHtmlTree.add_child("p", certLink);

                pt::ptree patentLink;
                patentLink.put("<xmlattr>.class", "spacing1");
                patentLink.put("<xmltext>", language == "de" ? "Grundpatent:" : "Brevet de base:");
                patentLink.put("a.<xmltext>", cert.basePatent);
                patentLink.put("a.<xmlattr>.href", "https://www.swissreg.ch/database-client/register/detail/patent/" + cert.basePatentId + "?lang=" + language);
                patentLink.put("a.<xmlattr>.target", "_blank");
                extraHtmlTree.add_child("p", patentLink);

                pt::ptree issueDate;
                issueDate.put("<xmlattr>.class", "spacing1");
                issueDate.put("<xmltext>", (language == "de" ? "Erteilungsdatum:" : "Date de délivrance:") + cert.issueDate);
                extraHtmlTree.add_child("p", issueDate);

                pt::ptree publicationDate;
                publicationDate.put("<xmlattr>.class", "spacing1");
                publicationDate.put("<xmltext>", (language == "de" ? "Publikationsdatum:" : "Date de la publication:") + cert.publicationDate);
                extraHtmlTree.add_child("p", publicationDate);

                pt::ptree protectionDate;
                protectionDate.put("<xmlattr>.class", "spacing1");
                protectionDate.put("<xmltext>", (language == "de" ? "Schutzdauerbeginn:" : "Début de la durée de protection:") + cert.protectionDate);
                extraHtmlTree.add_child("p", protectionDate);

                pt::ptree basePatentDate;
                basePatentDate.put("<xmlattr>.class", "spacing1");
                basePatentDate.put("<xmltext>", (language == "de" ? "Anmeldedatum:" : "Date de dépôt:") + cert.basePatentDate);
                extraHtmlTree.add_child("p", basePatentDate);

                pt::ptree expiryDate;
                expiryDate.put("<xmlattr>.class", "spacing1");
                expiryDate.put("<xmltext>", (language == "de" ? "Maximale Schutzdauer:" : "Durée maximale de protection:") + cert.expiryDate);
                extraHtmlTree.add_child("p", expiryDate);

                pt::ptree deletionDate;
                deletionDate.put("<xmlattr>.class", "spacing1");
                deletionDate.put("<xmltext>", (language == "de" ? "Löschung:" : "Radiation:") + cert.deletionDate);
                extraHtmlTree.add_child("p", deletionDate);
            }
        }
        if (!addedCertificateIds.empty()) {
            tree.add_child("html.body.div.div", extraHtmlTree);
            sectionIds.push_back(sectionId);
            sectionTitles.push_back(sectionName);
        }
    }

    // Add a section that was not in the XML contents
    // Note that this section id and name don't get added to the chapter name list
    // Footer
    {
        std::string extraHtml;
        extraHtml += "   <div class=\"paragraph\" id=\"Section" + std::to_string(SECTION_NUMBER_FOOTER) + "\"></div>\n";

        std::time_t seconds = std::time(nullptr);
        std::string curtime = std::asctime(std::localtime( &seconds )); // TODO: avoid trailing \n
        std::string url("https://github.com/zdavatz/");
        url += appName;
        extraHtml += "<p class=\"footer\">Auto-generated by <a href=\"" + url + "\">" + appName + "</a> on " + curtime + "</p>";
        std::stringstream ss;
        ss << extraHtml;
        pt::ptree extraHtmlTree;
        try {
            pt::read_xml(ss, extraHtmlTree, pt::xml_parser::no_concat_text);
        } catch (std::exception &e) {
            std::cerr << "Error reading footer html: " << extraHtml << std::endl;
            std::cerr << "Line: " << __LINE__ << " Error " << e.what() << std::endl;
        }

        BOOST_FOREACH(pt::ptree::value_type &et, extraHtmlTree) {
            tree.get_child("html.body.div").push_back(pt::ptree::value_type(et.first, et.second));
        }
    }

    {
        std::stringstream ss;
        pt::write_xml(ss, tree);
        xml = ss.str();
        boost::replace_all(xml, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n", "");

        // Need to add an empty head, as iOS client add style by replacing it
        // it must be done in string, as adding it in tree results in <head/> which doesn't hit the replace
        boost::replace_all(xml, "<html>", "<html><head></head>");
    }

    html = xml;
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

GTIN::oneFachinfoPackages fillPackagesInRow(
    std::string opt_language,
    DB::RowToInsert *rowToInsert,
    unsigned int *statsRnFoundRefdataCount,
    unsigned int *statsRnNotFoundRefdataCount,
    unsigned int *statsRnFoundSwissmedicCount,
    unsigned int *statsRnNotFoundSwissmedicCount,
    unsigned int *statsRnFoundBagCount,
    unsigned int *statsRnNotFoundBagCount,
    std::vector<std::string> *statsRegnrsNotFound
) {
    GTIN::oneFachinfoPackages packages;
    std::set<std::string> gtinUsedSet; // To ensure we don't have duplicates, and for stats

    std::vector<std::string> regnrs;
    boost::algorithm::split(regnrs, rowToInsert->regnrs, boost::is_any_of(", "), boost::token_compress_on);

    for (auto rn : regnrs) {
        //std::cerr << basename((char *)__FILE__) << ":" << __LINE__  << " rn: " << rn << std::endl;

        // Search in refdata
        int nAdd = REFDATA::getNames(rn, gtinUsedSet, packages);
        if (nAdd == 0){
            (*statsRnNotFoundRefdataCount)++;
        } else {
            (*statsRnFoundRefdataCount)++;
        }

        // Search in swissmedic
        nAdd = SWISSMEDIC::getAdditionalNames(rn, gtinUsedSet, packages, opt_language);
        if (nAdd == 0) {
            (*statsRnNotFoundSwissmedicCount)++;
        } else {
            (*statsRnFoundSwissmedicCount)++;
        }

        // Search in bag
        nAdd = BAG::getAdditionalNames(rn, gtinUsedSet, packages);
        if (nAdd == 0) {
            (*statsRnNotFoundBagCount)++;
        } else {
            (*statsRnFoundBagCount)++;
        }

        if (gtinUsedSet.empty()) {
            statsRegnrsNotFound->push_back(rn);
        }
    }
    BEAUTY::sort(packages);

    // Create a single multi-line string from the vector
    std::string packInfo = boost::algorithm::join(packages.name, "\n");

    if (!packInfo.empty()) {
        rowToInsert->pack_info_str = packInfo;
    }

    // packages
    // The line order must be the same as pack_info_str
    std::vector<std::string>::iterator itGtin = packages.gtin.begin();
    std::vector<std::string> lines;
    for (auto name : packages.name) {

        SWISSMEDIC::dosageUnits du = SWISSMEDIC::getByGtin(*itGtin);
        BAG::packageFields pf = BAG::getPackageFieldsByGtin(*itGtin);

        // Field 0
        // TODO: temporarily use the first part of the name
        std::string::size_type len = name.find(",");
        std::string oneLine = len != name.npos ? name.substr(0, len) : name;  // pos, len

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
    std::string packagesStr = boost::algorithm::join(lines, "\n");
    rowToInsert->packages = packagesStr;
    return packages;
}

void fillApplicationStr(DB::RowToInsert *rowToInsert) {
    std::vector<std::string> regnrs;
    boost::algorithm::split(regnrs, rowToInsert->regnrs, boost::is_any_of(", "), boost::token_compress_on);
    std::string application = SWISSMEDIC::getApplication(regnrs[0]);
    std::string appBag = BAG::getApplicationByRN(regnrs[0]);
    if (!appBag.empty()) {
        application += ";" + appBag;
    }

    if (!application.empty()) {
        rowToInsert->application_str = application;
    }
}

int main(int argc, char **argv)
{
    //std::setlocale(LC_ALL, "en_US.utf8");

    appName = basename(argv[0]);

    std::string opt_inputDirectory;
    std::string opt_workDirectory;  // for downloads subdirectory
    std::string opt_language;
    bool flagXml = false;
    bool flagVerbose = false;
    bool flagNoSappinfo = false;
    bool flagPinfo = false;
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
       ("pinfo", "generate patient info htmls") // Generate pi
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
        flagPinfo = true;
        type = "pi";
        // std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << " flagPinfo: " << flagPinfo << std::endl;
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

    PED::parseXML(opt_workDirectory + "/downloads/swisspeddosepublication_v4.xml",
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

    DRUGSHORTAGE::parseJSON(opt_workDirectory + "/downloads/drugshortage.json", opt_inputDirectory, opt_language);

    if (opt_language == "fr") {
        BATCHRECALLS::parseJSON(opt_workDirectory + "/downloads/chargenrueckrufe_fr.json");
    } else {
        BATCHRECALLS::parseJSON(opt_workDirectory + "/downloads/chargenrueckrufe_de.json");
    }

    if (opt_language == "fr") {
        DHPCHPC::parseJSON(opt_workDirectory + "/downloads/dhpc_hpc_fr.json");
    } else {
        DHPCHPC::parseJSON(opt_workDirectory + "/downloads/dhpc_hpc_de.json");
    }

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
    BAGFHIR::parseNDJSON(opt_workDirectory + "/downloads/fhir-sl.ndjson", opt_language, flagVerbose);
    {
        std::vector<std::string> bagList = BAGFHIR::getGtinList();
        REP::html_h4("Cross-reference (BAG FHIR)");
        REP::html_start_ul();
        REP::html_li(std::to_string(countBagGtinInSwissmedic(bagList)) + " GTIN are also in swissmedic");
        REP::html_li(std::to_string(countBagGtinInRefdata(bagList)) + " GTIN are also in refdata");
        REP::html_end_ul();
    }

    for (AIPS::Medicine& m : list) {
        std::vector<std::string> regnrs;
        boost::algorithm::split(regnrs, m.regnrs, boost::is_any_of(", "), boost::token_compress_on);
        for (std::string regnr : regnrs) {
            std::string swissAtc = SWISSMEDIC::getAtcFromFirstRn(regnr);
            std::string refdataAtc = REFDATA::findAtc(regnr);
            if (swissAtc.size() > refdataAtc.size()) {
                m.atc = swissAtc;
            } else if (swissAtc.size() < refdataAtc.size()) {
                m.atc = refdataAtc;
            } else {
                m.atc = swissAtc;
            }
            if (!m.atc.empty()) {
                m.atc += ";" + ATC::getTextByAtcs(m.atc);
                break;
            }
        }
        if (m.atc.empty()) {
            statsATCNotFound.push_back(m.regnrs);
        }
    }

    SWISSREG::parseJSON(opt_inputDirectory + "/swissreg.json");

    if (!flagNoSappinfo)
        SAPP::parseXLXS(opt_inputDirectory, "/sappinfo.xlsx", opt_language);

    if (flagXml) {
        std::cerr << "Creating XML not yet implemented" << std::endl;
    }
    else {
        std::string dbFilename = opt_workDirectory + "/output/amiko_db_full_idx_" + opt_language + ".db";
        if (flagPinfo) {
            dbFilename = opt_workDirectory + "/output/amiko_db_full_idx_pinfo_" + opt_language + ".db";
        }
        openDB(dbFilename);

        unsigned int statsRnFoundRefdataCount = 0;
        unsigned int statsRnNotFoundRefdataCount = 0;
        unsigned int statsRnFoundSwissmedicCount = 0;
        unsigned int statsRnNotFoundSwissmedicCount = 0;
        unsigned int statsRnFoundBagCount = 0;
        unsigned int statsRnNotFoundBagCount = 0;
        std::vector<std::string> statsRegnrsNotFound;

        std::set<std::string> addedRegnrs;

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
            for (std::string regnr : regnrs) {
                addedRegnrs.insert(regnr);
            }

            if (regnrs[0] == "00000")
                continue;

            DB::RowToInsert rowToInsert;

            // See DispoParse.java:164 addArticleDB()
            // See SqlDatabase.java:347 addExpertDB()
            rowToInsert.title = m.title;
            rowToInsert.auth = m.auth;
            rowToInsert.atc = m.atc;
            rowToInsert.substances = m.subst;
            rowToInsert.regnrs = m.regnrs;

            // atc_class
            std::string atcClass = ATC::getClassByAtcColumn(m.atc);
            rowToInsert.atc_class = atcClass;

            // tindex_str
            std::string tindex = BAG::getTindex(regnrs[0]);
            if (tindex.empty())
                rowToInsert.tindex_str = "";
            else
                rowToInsert.tindex_str = tindex;

            fillApplicationStr(&rowToInsert);

#if 1

            GTIN::oneFachinfoPackages packages = fillPackagesInRow(
                opt_language,
                &rowToInsert,
                &statsRnFoundRefdataCount,
                &statsRnNotFoundRefdataCount,
                &statsRnFoundSwissmedicCount,
                &statsRnNotFoundSwissmedicCount,
                &statsRnFoundBagCount,
                &statsRnNotFoundBagCount,
                &statsRegnrsNotFound
            );

#endif

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

                getHtmlFromXml(m.contentHTMLPath, html, m.regnrs, m.auth,
                               packages,        // for barcodes
                               sectionId,       // for ids_str
                               sectionTitle,    // for titles_str
                               firstAtc,        // for pedDose
                               opt_language,    // for barcode section
                               flagVerbose,
                               flagNoSappinfo);
                rowToInsert.content = html;
            }

            // ids_str
            {
                std::string ids_str = boost::algorithm::join(sectionId, ",");
                rowToInsert.ids_str = ids_str;
            }

            // titles_str
            {
                std::string titles_str = boost::algorithm::join(sectionTitle, TITLES_STR_SEPARATOR);
                rowToInsert.titles_str = titles_str;
            }

            sqlDb.insertRow(TABLE_NAME_AMIKO, rowToInsert);
        } // for

#ifdef WITH_PROGRESS_BAR
        std::cerr << "\r100 %" << std::endl;
#endif

        if (flagPinfo) {

            // Preparations.xml (BAG) and Packungen.xlsx (Swissmedic).
            std::vector<BAG::Preparation> bagPrepList = BAG::getPrepList();
            for (auto bagPrep : bagPrepList) {
                if (addedRegnrs.find(bagPrep.swissmedNo) == addedRegnrs.end()) {
                    DB::RowToInsert rowToInsert;

                    std::string auth = "";
                    for (auto pack : bagPrep.packs) {
                        if (!pack.partnerDescription.empty()) {
                            auth = pack.partnerDescription;
                            break;
                        }
                    }

                    rowToInsert.title = bagPrep.name;
                    rowToInsert.auth = auth;
                    rowToInsert.atc = bagPrep.atcCode;
                    // rowToInsert.substances;
                    rowToInsert.regnrs = bagPrep.swissmedNo;
                    std::string atcClass = ATC::getClassByAtcColumn(bagPrep.atcCode);
                    rowToInsert.atc_class = atcClass;
                    rowToInsert.tindex_str = bagPrep.itCodes.tindex;
                    fillApplicationStr(&rowToInsert);
                    fillPackagesInRow(
                        opt_language,
                        &rowToInsert,
                        &statsRnFoundRefdataCount,
                        &statsRnNotFoundRefdataCount,
                        &statsRnFoundSwissmedicCount,
                        &statsRnNotFoundSwissmedicCount,
                        &statsRnFoundBagCount,
                        &statsRnNotFoundBagCount,
                        &statsRegnrsNotFound
                    );

                    sqlDb.insertRow(TABLE_NAME_AMIKO, rowToInsert);
                    addedRegnrs.insert(rowToInsert.regnrs);
                }
            }

            for (std::string swissRegnr : SWISSMEDIC::getRegnrs()) {
                if (addedRegnrs.find(swissRegnr) == addedRegnrs.end()) {
                    DB::RowToInsert rowToInsert = SWISSMEDIC::getRow(swissRegnr);
                    std::string atcClass = ATC::getClassByAtcColumn(rowToInsert.atc);
                    rowToInsert.atc_class = atcClass;
                    fillApplicationStr(&rowToInsert);
                    fillPackagesInRow(
                        opt_language,
                        &rowToInsert,
                        &statsRnFoundRefdataCount,
                        &statsRnNotFoundRefdataCount,
                        &statsRnFoundSwissmedicCount,
                        &statsRnNotFoundSwissmedicCount,
                        &statsRnFoundBagCount,
                        &statsRnNotFoundBagCount,
                        &statsRegnrsNotFound
                    );
                    sqlDb.insertRow(TABLE_NAME_AMIKO, rowToInsert);
                    addedRegnrs.insert(rowToInsert.regnrs);
                }
            }
        }

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
        REP::html_li("ATC not found: " + std::to_string(statsATCNotFound.size()));
        REP::html_end_ul();
        if (statsATCNotFound.size() > 0)
            REP::html_div(boost::algorithm::join(statsATCNotFound, ", "));

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
        BAGFHIR::printUsageStats();
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
