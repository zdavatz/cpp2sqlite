//
//  epha.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 31 Jan 2019
//

#include <iostream>
#include <map>
#include <libgen.h>     // for basename()

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "epha.hpp"

//#define DEBUG

namespace pt = boost::property_tree;

namespace EPHA
{
    std::map<int, Document> docMap;
    unsigned int statsTotalDocCount = 0;
    unsigned int statsTotalDocWithAtcCount = 0;
    unsigned int statsTotalDocWithoutAtcCount = 0;
    unsigned int statsTotalRnCount = 0;

void parseJSON(const std::string &filename, bool verbose)
{
    pt::ptree tree;
    
    try {
        std::clog << "Reading epha JSON" << std::endl;
        pt::read_json(filename, tree);
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
    std::cerr << "Analyzing epha" << std::endl;

    BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("documents")) {
        statsTotalDocCount++;
        // Example 1: "zulassung": "00294",
        // Example 2: "zulassung": "00295 00296",
        // Example 3: "zulassung": "62746 62747 62848 62849 62850 62851 62852"
        // Example 4: "zulassung": "61690..." 136 of them !
        std::string rns = v.second.get("zulassung", "");
        // Break it down in individual rn
        std::vector<std::string> rnVector;
        boost::algorithm::split(rnVector, rns, boost::is_any_of(" "), boost::token_compress_on);

        std::string sub = v.second.get("substanz", "");
        std::string her = v.second.get("hersteller", "");
        std::string atc = v.second.get("atc", "");
        std::string pro = v.second.get("produkt", "");
        
        if (atc.empty()) {
            if (verbose) {
                std::clog << basename((char *)__FILE__) << ":" << __LINE__;
                if (rnVector.size() == 1)
                    std::clog << " No ATC for rn " << rns;
                else
                    std::clog << " No ATC for these " << rnVector.size() << " regnrs: " << rns;
                
                std::clog << std::endl;
            }

            statsTotalDocWithoutAtcCount++;
            continue;
        }
        
        statsTotalDocWithAtcCount++;

#if 0 //def DEBUG
        if (rnVector.size() > 1)
            std::clog
            << basename((char *)__FILE__) << ":" << __LINE__
            << ", size: " << rnVector.size()
            << ", atc: " << atc
            << ", regnrs: " << rns
            << std::endl;
#endif

        for (auto rn : rnVector) {
            statsTotalRnCount++;
            Document doc {sub, her, atc, pro};
            docMap.insert(std::make_pair(std::stoi(rn), doc));
        }
    }
    
    std::clog
    << basename((char *)__FILE__) << ":" << __LINE__
    << ", # documents: " << statsTotalDocCount
    << " (with atc: " << statsTotalDocWithAtcCount
    << ", without atc: " << statsTotalDocWithoutAtcCount << ")"
    << ", # distinct rn: " << statsTotalRnCount
    << std::endl;
}

std::string getAtcFromSingleRn(const std::string &rn)
{
    std::string atc;
    Document doc;
    auto search = docMap.find(std::stoi(rn));
    if (search != docMap.end()) {
        doc = search->second;
        atc = doc.atc;
    }

#ifdef DEBUG
    std::clog
    << basename((char *)__FILE__) << ":" << __LINE__
    << ", rn: " << rn
    << ", atc: " << atc
    << std::endl;
#endif
    
    return atc;
}

}
