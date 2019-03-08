//
//  main.cpp
//  interaction
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 8 Mar 2019
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <libgen.h>     // for basename()

#include <boost/algorithm/string_regex.hpp>

#define OUTPUT_FILE_SEPARATOR   "|"

#define inColumnA     cols[0]
#define inColumnB     cols[1]
#define inColumnC     cols[2]
#define inColumnD     cols[3]
#define inColumnE     cols[4]
#define inColumnF     cols[5]
#define inColumnG     cols[6]
#define inColumnH     cols[7]
#define inColumnI     cols[8]

void outputInteraction(std::ofstream &ofs,
                       const std::vector<std::string> &cols)
{
    if (cols.size() != 9) {
        std::clog << "Unexpected # columns: " << cols.size() << std::endl;
        return;
    }

    std::string nameSection1 = "Risikoklasse";
    std::string nameSection2 = "Möglicher Effekt";
    std::string nameSection3 = "Mechanismus";
    std::string nameSection4 = "Empfohlene Massnahmen";
    
    std::string legend; // TODO: localize
    if (inColumnI == "A")
        legend = "Keine Massnahmen notwendig";
    else if (inColumnI == "B")
        legend = "Vorsichtsmassnahmen empfohlen";
    else if (inColumnI == "C")
        legend = "Regelmässige Überwachung";
    else if (inColumnI == "D")
        legend = "Kombination vermeiden";
    else if (inColumnI == "X")
        legend = "Kontraindiziert";
    else
        legend = "Unknown section";

    std::string outColumnE = "<div class=\"paragraph" + inColumnI + "\" id=\"" + inColumnA + "-" + inColumnC + "\">";
    outColumnE += "<div class=\"absTitle\">" + inColumnA + " [" + inColumnB + "] &rarr; " + inColumnC + " [" + inColumnD + "]</div>";
    outColumnE += "</div>";
    outColumnE += "<p class=\"spacing2\"><i>" + nameSection1 + ":</i> " + legend + " (" + inColumnI + ")</p>";
    outColumnE += "<p class=\"spacing2\"><i>" + nameSection2 + ":</i> " + inColumnE + "</p>";
    outColumnE += "<p class=\"spacing2\"><i>" + nameSection3 + ":</i> " + inColumnF + "</p>";
    outColumnE += "<p class=\"spacing2\"><i>" + nameSection4 + ":</i> " + inColumnH + "</p>";

    outColumnE = "<div>" + outColumnE + "</div>";
#if 0
    std::clog
#else
    ofs
#endif
    << inColumnA << OUTPUT_FILE_SEPARATOR   // A
    << OUTPUT_FILE_SEPARATOR                // B empty column
    << inColumnC << OUTPUT_FILE_SEPARATOR   // C
    << OUTPUT_FILE_SEPARATOR                // D empty column
    << outColumnE
    << std::endl;
}

void parseCSV(const std::string &filename,
              const std::string &language,
              bool verbose)
{
    std::ofstream ofs;
    
    std::string logDir("./");
    std::string fullFilename(logDir);
    fullFilename += "drug_interaction_" + language + ".csv";
    ofs.open(fullFilename.c_str());

    try {
        //std::clog << std::endl << "Reading CSV" << std::endl;
        std::ifstream file(filename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str)) {
            
            std::vector<std::string> columnVector;
            std::vector<std::string> columnTrimmedVector;
            //boost::algorithm::split(interVector, str, boost::is_any_of("\","), boost::token_compress_on);
            boost::algorithm::split_regex( columnVector, str, boost::regex( "\"," ));

            for (auto s : columnVector) {
                boost::algorithm::trim_left_if(s, boost::is_any_of("\""));
                boost::algorithm::trim_right_if(s, boost::is_any_of("\""));
                columnTrimmedVector.push_back(s);
            }

            if (header) {
                header = false;
                continue;
            }
            
            outputInteraction(ofs, columnTrimmedVector);
        }
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
    ofs.close();
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cout << "Missing CSV filename" << std::endl;
        return EXIT_FAILURE;
    }
    
    parseCSV(argv[1], "de", false);
    return EXIT_SUCCESS;
}
