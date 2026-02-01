//
//  refdata.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 21 Jan 2019
//

#ifndef refdata_hpp
#define refdata_hpp

#include <iostream>
#include <vector>
#include <boost/property_tree/ptree.hpp>

#include "medicine.h"
#include "gtin.hpp"

namespace REFDATA
{
    namespace pt = boost::property_tree;
    struct Article {
        std::string gtin_13;
        std::string gtin_5;
        std::string phar;
        std::string name;
        std::string atc;
        std::string authorisation_identifier;
    };

    struct ArticleSectionParagraph {
        bool needs_to_wrap_in_paragraph = false;
        pt::ptree tree;
        bool is_italic;
    };

    struct ArticleSection {
        std::string title;
        std::string id;
        std::vector<ArticleSectionParagraph> paragraphs;
    };

    struct ArticleDocument {
        std::vector<ArticleSection> sections;
    };

    typedef std::vector<Article> ArticleList;

    void parseXML(const std::string &filename,
                  const std::string &language);

    int getNames(const std::string &rn,
                 std::set<std::string> &gtinUsed,
                 GTIN::oneFachinfoPackages &packages);

    bool findGtin(const std::string &gtin);

    std::string getPharByGtin(const std::string &gtin);

    void printUsageStats();

    std::string findAtc(const std::string &regnrs);
    std::string findName(const std::string &regnrs);

    void findSectionIdsAndTitle(
        pt::ptree tree,
        std::vector<std::string> &sectionIds,
        std::vector<std::string> &sectionTitles
    );

    ArticleDocument getArticleDocument(std::string path);
}

#endif /* refdata_hpp */
