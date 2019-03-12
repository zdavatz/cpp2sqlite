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
#include <set>
#include <map>
#include <libgen.h>     // for basename()

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string_regex.hpp>

#include "config.h"
#include "atc.hpp"

////////////////////////////////////////////////////////////////////////////////
#include "lang/en.h"    // Base language
#include "lang/de.h"
#include "lang/fr.h"

////////////////////////////////////////////////////////////////////////////////
#define OUTPUT_FILE_SEPARATOR   "|"

#define inColumnA     columnTrimmedVector[0] // ATC1
#define inColumnB     columnTrimmedVector[1] // Name1
#define inColumnC     columnTrimmedVector[2] // ATC2
#define inColumnD     columnTrimmedVector[3] // Name2
#define inColumnE     columnTrimmedVector[4] // Info
#define inColumnF     columnTrimmedVector[5] // Mechanismus
#define inColumnG     columnTrimmedVector[6] // Effekt  ( unused ? )
#define inColumnH     columnTrimmedVector[7] // Massnahmen
#define inColumnI     columnTrimmedVector[8] // Grad

namespace po = boost::program_options;

static std::string appName;

// Used when language == "de"
std::set<std::string> toBeTranslatedSet; // no duplicates, sorted

// Used when language != "de"
std::map<std::string, std::string> translatedMap;

void on_version()
{
    std::cout << appName << " " << PROJECT_VER
    << ", " << __DATE__ << " " << __TIME__ << std::endl;
    
    std::cout << "C++ " << __cplusplus << std::endl;
}

void outputInteraction(std::ofstream &ofs,
                       const std::vector<std::string> &columnTrimmedVector)
{
    if (columnTrimmedVector.size() != 9) {
        std::clog << "Unexpected # columns: " << columnTrimmedVector.size() << std::endl;
        return;
    }
    
    std::string legend("Unknown section");
    if      (inColumnI == "A") legend = legendA;
    else if (inColumnI == "B") legend = legendB;
    else if (inColumnI == "C") legend = legendC;
    else if (inColumnI == "D") legend = legendD;
    else if (inColumnI == "X") legend = legendX;

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

// Define translatedMap
// key "input/deepl.in.txt"
// val "input/deepl.out.fr.txt"
void getTranslationMap(const std::string &dir,
                       const std::string &language)
{
    try {
        std::ifstream ifsKey(dir + "/deepl.in.txt");
        std::ifstream ifsValue(dir + "/deepl.out." + language + ".txt");   // translated by deepl.sh
        std::ifstream ifsValue2(dir + "/deepl.out2." + language + ".txt"); // translated manually

        std::string key, val;
        while (std::getline(ifsKey, key)) {
            
            std::getline(ifsValue, val);
            if (val.empty())                    // DeepL failed to translate it
                std::getline(ifsValue2, val);   // Get it from manually translated file

            translatedMap.insert(std::make_pair(key, val));
        }
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
}

void parseCSV(const std::string &inFilename,
              const std::string &outDir,
              const std::string &language,
              bool verbose)
{
    std::ofstream ofs;
    ofs.open(outDir + "/drug_interactions_csv_" + language + ".csv");

    try {
        //std::clog << std::endl << "Reading CSV" << std::endl;
        std::ifstream file(inFilename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str)) {
            
            if (header) {
                header = false;
                continue;
            }
            
            std::vector<std::string> columnVector;
            std::vector<std::string> columnTrimmedVector;
            //boost::algorithm::split(interVector, str, boost::is_any_of("\","), boost::token_compress_on);
            boost::algorithm::split_regex( columnVector, str, boost::regex( "\"," ));

            if (columnVector.size() != 9) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }

            for (auto s : columnVector) {
                boost::algorithm::trim_left_if(s, boost::is_any_of("\""));
                boost::algorithm::trim_right_if(s, boost::is_any_of("\""));
                columnTrimmedVector.push_back(s);
            }

            if (language == "de") {
                outputInteraction(ofs, columnTrimmedVector);

                toBeTranslatedSet.insert(inColumnE);
                toBeTranslatedSet.insert(inColumnF);
                toBeTranslatedSet.insert(inColumnH);
            }
            else {
                std::vector<std::string> translatedVector;
                
                translatedVector.push_back(inColumnA);
                translatedVector.push_back(ATC::getTextByAtc(inColumnB));
                translatedVector.push_back(inColumnC);
                translatedVector.push_back(ATC::getTextByAtc(inColumnD));
                translatedVector.push_back(translatedMap[inColumnE]);
                translatedVector.push_back(translatedMap[inColumnF]);
                translatedVector.push_back(inColumnG);
                translatedVector.push_back(translatedMap[inColumnH]);
                translatedVector.push_back(inColumnI);

                outputInteraction(ofs, translatedVector);
            }
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
    appName = boost::filesystem::basename(argv[0]);
    
    std::string opt_inputDirectory;
    std::string opt_workDirectory;  // for downloads subdirectory
    std::string opt_language;
    bool flagVerbose = false;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "print this message")
        ("version,v", "print the version information and exit")
        ("verbose", "be extra verbose") // Show errors and logs
        ("lang", po::value<std::string>( &opt_language )->default_value("de"), "use given language (de/fr)")
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
    
    if (!vm.count("workDir")) {
        opt_workDirectory = opt_inputDirectory + "/..";
    }

    ////////////////////////////////////////////////////////////////////////////
    if (opt_language == "de") {
        stringsFromDe();
    }
    else if (opt_language == "fr") {
        stringsFromFr();

        // For French names of medicines
        ATC::parseTXT(opt_inputDirectory + "/atc_codes_multi_lingual.txt", opt_language, flagVerbose);
    }

    if (opt_language != "de") {
        getTranslationMap(opt_inputDirectory, opt_language);
    }

    parseCSV(opt_inputDirectory + "/matrix.csv",
             opt_workDirectory + "/output",
             opt_language,
             false);

    if (opt_language == "de") {
        std::string toBeTran = boost::algorithm::join(toBeTranslatedSet, "\n");
        std::ofstream outfile(opt_inputDirectory + "/deepl.in.txt");
        outfile << toBeTran;
        outfile.close();
    }
    
    return EXIT_SUCCESS;
}
