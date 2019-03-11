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
#include <boost/asio/io_service.hpp>

#define OUTPUT_FILE_SEPARATOR   "|"

#define inColumnA     cols[0] // ATC1
#define inColumnB     cols[1] // Name1
#define inColumnC     cols[2] // ATC2
#define inColumnD     cols[3] // Name2
#define inColumnE     cols[4] // Info
#define inColumnF     cols[5] // Mechanismus
#define inColumnG     cols[6] // Effekt  ( unused ? )
#define inColumnH     cols[7] // Massnahmen
#define inColumnI     cols[8] // Grad

std::set<std::string> toBeTranslatedSet;

void outputInteraction(std::ofstream &ofs,
                       const std::vector<std::string> &cols)
{
    if (cols.size() != 9) {
        std::clog << "Unexpected # columns: " << cols.size() << std::endl;
        return;
    }

    toBeTranslatedSet.insert(inColumnE);
    toBeTranslatedSet.insert(inColumnF);
    toBeTranslatedSet.insert(inColumnH);

    // TODO: localize
    std::string nameSection1 = "Risikoklasse";          // Risk class
    std::string nameSection2 = "Möglicher Effekt";      // Possible effect
    std::string nameSection3 = "Mechanismus";           // Mechanism
    std::string nameSection4 = "Empfohlene Massnahmen"; // Recommended measures
    
    std::string legend; // TODO: localize
    if (inColumnI == "A")
        legend = "Keine Massnahmen notwendig"; // No measures necessary
    else if (inColumnI == "B")
        legend = "Vorsichtsmassnahmen empfohlen";  // precautions recommended
    else if (inColumnI == "C")
        legend = "Regelmässige Überwachung";  // Regular monitoring
    else if (inColumnI == "D")
        legend = "Kombination vermeiden"; // Avoid combination
    else if (inColumnI == "X")
        legend = "Kontraindiziert"; // Contraindicated
    else
        legend = "Unknown section";

    std::string outColumnE = "<div class=\"paragraph" + inColumnI + "\" id=\"" + inColumnA + "-" + inColumnC + "\">";
    outColumnE += "<div class=\"absTitle\">" + inColumnA + " [" + inColumnB + "] &rarr; " + inColumnC + " [" + inColumnD + "]</div>";
    outColumnE += "</div>";
    outColumnE += "<p class=\"spacing2\"><i>" + nameSection1 + ":</i> " + legend + " (" + inColumnI + ")</p>"; // Grad
    outColumnE += "<p class=\"spacing2\"><i>" + nameSection2 + ":</i> " + inColumnE + "</p>";  // Info
    outColumnE += "<p class=\"spacing2\"><i>" + nameSection3 + ":</i> " + inColumnF + "</p>";  // Mechanismus
    outColumnE += "<p class=\"spacing2\"><i>" + nameSection4 + ":</i> " + inColumnH + "</p>";  // Massnahmen

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
    << outColumnE                           // E
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
    
    std::string toBeTran = boost::algorithm::join(toBeTranslatedSet, "\n");
    std::ofstream outfile ("deepl.in.txt");
    outfile << toBeTran;
    outfile.close();
    
    return EXIT_SUCCESS;
}
