//
//  aips.cpp
//  pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 6 Jun 2019
//

#include <iostream>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "aips.hpp"
#include "beautify.hpp"

enum sm_states {
    SM_BEFORE,
    SM_MIDDLE,  // Section of interest to be extracted
    SM_AFTER
};

namespace pt = boost::property_tree;

namespace AIPS
{
    std::map<std::string, std::string> storageMap;

// See getHtmlFromXml
static std::string getSections(const std::string &xml)
{
    pt::ptree tree;
    std::stringstream ss;
    ss << xml;
    pt::read_xml(ss, tree);

    int sm = SM_BEFORE;
    std::string storageText;

    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("div")) {

            std::string tagContent = v.second.data();

            if (v.first == "p")
            {
                bool isSection = true;
                std::string section;
                try {
                    section = v.second.get<std::string>("<xmlattr>.id");
                }
                catch (std::exception &e) {
                    isSection = false;
                    //std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error " << e.what() << std::endl;
                }  // try section

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

                if (needItalicSpan)
                {
                    switch (sm) {
                        case SM_BEFORE:
                            if (boost::contains(tagContent, "Lagerungshinweise"))
                                sm = SM_MIDDLE;

                            break;

                        case SM_MIDDLE:
                            sm = SM_AFTER;
                            return storageText;
                            break;

                        case SM_AFTER:
                            break;
                    }
                }
                else {
                    if (sm == SM_MIDDLE)
                        storageText += tagContent + "\n"; // Keep appending until a new section starts
                }
            }
        } // BOOST div
    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error " << e.what() << std::endl;
    }

    return storageText;
}

void parseXML(const std::string &filename,
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
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("MedicinalDocumentsBundles")) { // TODO
            if (v.first != "MedicinalDocumentsBundle") continue;
            std::string typ;
            // Mapping: https://github.com/zdavatz/cpp2sqlite/issues/252
            if (v.second.get("Type", "") == "SmPC") {
                typ = "fi";
            } else if (v.second.get("Type", "") == "PIL") {
                typ = "pi";
            }
            if (typ != type) {
                continue;
            }

            std::string regnrs = v.second.get("RegulatedAuthorization.Identifier", "");
            std::vector<std::string> rnVector;
            boost::algorithm::split(rnVector, regnrs, boost::is_any_of(", "), boost::token_compress_on);

            std::string content;

            BOOST_FOREACH(pt::ptree::value_type &attachedDocument, v.second) {
                if (attachedDocument.first != "AttachedDocument") {
                    continue;
                }
                std::string lan = attachedDocument.second.get("Language", "");

                if (lan != language) {
                    continue;
                }

                BOOST_FOREACH(pt::ptree::value_type &documentReference, attachedDocument.second) {
                    if (documentReference.first != "DocumentReference") continue;
                    if (documentReference.second.get("ContentType", "") != "text/html") continue;
                    std::string url = documentReference.second.get("Url", "");
                    std::string htmlFilename = std::filesystem::path(url).filename();
                    std::string downloadFolderPath = std::filesystem::path(filename).parent_path();
                    std::string htmlPath = downloadFolderPath + "/Refdata-AllHtml/" + htmlFilename;
                    std::ifstream in(htmlPath, std::ios::in | std::ios::binary);
                    if (in)
                    {
                        content = (std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
                    } else {
                        std::clog << "File not found: " << htmlPath << std::endl;
                    }
                    break;
                }
            }

            if (boost::contains(content, "Lagerungshinweise")) {

                BEAUTY::cleanupXml(content, regnrs);  // and escape some children tags

                // Get storage info from XML
                std::string storageInfo = getSections(content);
                if (!storageInfo.empty()) {
#if 0 //def DEBUG
                    std::cout
                    << "Line: " << __LINE__
                    << ", storage info: " << storageInfo
                    << std::endl;
#endif
                    for (auto rn : rnVector) {
                        storageMap.insert(std::make_pair(rn,storageInfo));
                    }
                }
            }
        }
    }
    catch (std::exception &e) {
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", Error " << e.what() << std::endl;
    }
}

std::string getStorageByRN(const std::string &rn)
{
    return storageMap[rn];
}

}
