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

namespace GENERIKA
{
constexpr std::string_view CSV_SEPARATOR = ";";

// Parse-phase stats
unsigned int statsNumDataLines = 0;

std::vector<std::string> eanVec;

// Adapted from http://mybyteofcode.blogspot.com/2010/11/parse-csv-file-with-embedded-new-lines.html
// Retain newlines if they are within double quotes
void getCsvLine(std::ifstream &file, std::string &line)
{
    bool inside_quotes(false);
    size_t last_quote(0);

    line.clear();
    std::string buffer;
    while (std::getline(file, buffer))
    {
        last_quote = buffer.find_first_of('"');
        while (last_quote != std::string::npos)
        {
            inside_quotes = !inside_quotes;
            last_quote = buffer.find_first_of('"', last_quote + 1);
        }

        line.append(buffer);

        if (inside_quotes) {
            //line.append("\n");
            continue;
        }
        
        break;
    }
    
    boost::algorithm::trim_right_if(line, boost::is_any_of("\n\r"));
}

void parseCSV(const std::string &filename, bool dumpHeader)
{
    std::clog << std::endl << "Reading " << filename << std::endl;

    try {
        std::ifstream file(filename);

        std::string header;
        getCsvLine(file, header);

        if (dumpHeader) {
            std::ofstream outHeader(filename + ".header.txt");
            std::vector<std::string> headerTitles;
            boost::algorithm::split(headerTitles, header, boost::is_any_of(CSV_SEPARATOR));
            outHeader << "Number of columns: " << headerTitles.size() << std::endl;
            auto colLetter = 'A';
            for (auto t : headerTitles)
                outHeader << colLetter++ << "\t" << t << std::endl;
            
            outHeader.close();
        }

        std::string str;

        // Problem: some cells in the header contain CRLF (admittedly withih "")
        while (std::getline(file, str))
        {            
            boost::algorithm::trim_right_if(str, boost::is_any_of("\n\r"));
            statsNumDataLines++;
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(CSV_SEPARATOR));
            
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
        
        file.close();
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
