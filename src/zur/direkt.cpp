//
//  direkt.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 1 Nov 2019
//

#include <iostream>
#include <vector>
#include <fstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <libgen.h>     // for basename()

#include "direkt.hpp"

namespace pt = boost::property_tree;

struct directArticle {
    std::string initial;
    std::string replacement;
};

namespace DIREKT
{

std::vector<directArticle> articleVec;

// Header: 1 line
// 2 columns
void parseCSV(const std::string &filename)
{
    std::clog << std::endl << "Reading " << filename << std::endl;
    
    try {
        std::ifstream file(filename);
        
        std::string str;
        bool header = true;

        while (std::getline(file, str))
        {
            if (header) {
                header = false;
                continue;
            }
            
            boost::algorithm::trim_right_if(str, boost::is_any_of("\n\r"));

            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(";"));
            
            if (columnVector.size() != 2) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }
            
            directArticle da {columnVector[0], columnVector[1]}; // initial, replacement
            articleVec.push_back(da);

        } // while
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
    std::sort(articleVec.begin(),
              articleVec.end(),
              [](directArticle const &a, directArticle const &b) {
                    return a.initial < b.initial;
                }
              );

#ifdef DEBUG
    std::clog << "Parsed " << articleVec.size() <<  " articles" << std::endl;
#endif
}

void createJSON(const std::string &filename)
{
    std::clog << "Writing " << filename << std::endl;

    pt::ptree tree;

    for (auto a : articleVec)
        tree.put(a.initial, a.replacement);
    
    pt::write_json(filename, tree);
}

}
