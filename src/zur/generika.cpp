//
//  generika.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 1 Nov 2019
//

#include <iostream>
#include <fstream>
#include <string>

#include <boost/algorithm/string.hpp>

#include <libgen.h>     // for basename()

#include "generika.hpp"

#define CREATE_JSON_WITHOUT_BOOST

#define WORKAROUND_CSV_HEADER_WITH_CRLF

#ifdef WORKAROUND_CSV_HEADER_WITH_CRLF
#define CSV_HEADER_LINES     5
#else
#define CSV_HEADER_LINES     1
#endif

namespace GENERIKA
{

// Parse-phase stats
unsigned int statsNumDataLines = 0;

std::vector<std::string> eanVec;

void parseCSV(const std::string &filename)
{
    std::clog << "Reading " << filename << std::endl;

    try {
        std::ifstream file(filename);
        
        std::string str;
        int headerLines = CSV_HEADER_LINES;

        // Problem: some cells in the header contain CRLF (admittedly withih "")
        while (std::getline(file, str))
        {
            if (headerLines-- > 0)
                continue;
            
            statsNumDataLines++;
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(";"));
            
            if (columnVector.size() != 15) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }
            
#ifdef CREATE_JSON_WITHOUT_BOOST
            std::string ean_code = "    \"" + columnVector[10] + "\""; // column K
#else
            std::string ean_code = columnVector[10]; // column K
#endif
            eanVec.push_back(ean_code);
        }
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }

#ifdef DEBUG
    std::clog << "Parsed " << statsNumDataLines << " lines" << std::endl;
#endif
}

void createJSON(const std::string &filename)
{
    std::clog << "Writing " << filename << std::endl;
    
    // The structure is so simple that we just treat it as a plain text file

    std::ofstream outfile(filename);
    outfile << "[" << std::endl;

    std::string joinedVec = boost::algorithm::join(eanVec, ",\n");
    outfile << joinedVec << std::endl;;

    outfile << "]" << std::endl;
    outfile.close();
}

}
