//
//  gripp.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 5 Dec 2020
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <libgen.h>     // for basename()

#include "gripp.hpp"

namespace GRIPP
{
constexpr std::string_view CSV_SEPARATOR = ";";

std::vector<std::string> grippVec;

// Parse-phase stats
unsigned int statsGrippNumLines = 0;

void parseCSV(const std::string &filename)
{
    std::clog << "Reading " << filename << std::endl;

    try {
        std::ifstream file(filename);

        std::string str;

        while (std::getline(file, str))
        {
            boost::algorithm::trim_right_if(str, boost::is_any_of("\n\r"));
            statsGrippNumLines++;

            // No header

            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(CSV_SEPARATOR));

            auto n = columnVector.size();
            if (n == 0) {
                std::clog << "Unexpected # columns: " << n << std::endl;
                exit(EXIT_FAILURE);
            }
            grippVec.push_back(columnVector[0]);
        }  // while

        file.close();
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }

#ifdef DEBUG
    std::clog
    << "Parsed " << statsGrippNumLines << " lines"
    << "\n\t" << grippVec.size() << " rows"
    << std::endl;
#endif
}

void createJSON(const std::string &filename)
{
    std::clog << "Writing " << filename << std::endl;

    // The structure is so simple that we just treat it as a plain text file

    std::ofstream outfile(filename);
    outfile << "[" << std::endl;

    std::string joinedVec = boost::algorithm::join(grippVec, ",\n");
    outfile << joinedVec << std::endl;;

    outfile << "]" << std::endl;
    outfile.close();
}
}
