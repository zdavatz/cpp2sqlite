//
//  main.cpp
//  drugshortage
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 25 Jan 2021
//
// Purpose:
//  Extract German strings from the input file and
//  write them out to an output file to be fed separately to
//  the translation engine DeepL

#include <iostream>
#include <string>
#include <set>
#include <unordered_set>
#include <libgen.h>     // for basename()

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string_regex.hpp>

#include <nlohmann/json.hpp>
#include "config.h"

namespace po = boost::program_options;
static std::string appName;

#pragma mark - DEEPL

namespace DEEPL
{
std::set<std::string> toBeTranslatedSet; // no duplicates, sorted

void validateAndAdd(const std::string s)
{
    if (s.empty())
        return;

    // Skip if it starts with a number
    // sheet 1, column I, could be like "120mg" or "keine Angaben"
    if (std::isdigit(s[0]))
        return;
    
    // Sometimes it start with a number, but after a space: " 80mg"
    // example sheet 2, column K, ATC C05AD01
    if (std::isspace(s[0]) && std::isdigit(s[1]))
        return;

    // Treat this as an empty cell
    if (s == "-")
        return;

    toBeTranslatedSet.insert(s);
}

void parseJSON(const std::string &inFilename,
               const std::string &outDir,
               bool verbose)
{
    std::clog << std::endl << "Reading " << inFilename << std::endl;
    std::ifstream jsonInputStream(inFilename);
    nlohmann::json drugshortageJson;
    jsonInputStream >> drugshortageJson;
    for (nlohmann::json::iterator it = drugshortageJson.begin(); it != drugshortageJson.end(); ++it) {
        auto entry = it.value();
        try {
            if (entry.contains("entry")) {
                validateAndAdd(entry["status"].get<std::string>());
            }
            if (entry.contains("datumLieferfahigkeit")) {
                validateAndAdd(entry["datumLieferfahigkeit"].get<std::string>());
            }
            if (entry.contains("datumLetzteMutation")) {
                validateAndAdd(entry["datumLetzteMutation"].get<std::string>());
            }
            if (entry.contains("colorCode")) {
                auto cEntry = entry["colorCode"];
                if (cEntry.contains("Bewertung")) {
                    validateAndAdd(cEntry["Bewertung"]);
                }
                if (cEntry.contains("Art der Meldung")) {
                    validateAndAdd(cEntry["Art der Meldung"]);
                }
            }
        }catch(std::exception &err){
            std::cerr << "Error: " << err.what() << std::endl;
        }
    }
}

} // namespace DEEPL

#pragma mark -

void on_version()
{
    std::cout << appName << " " << PROJECT_VER
    << ", " << __DATE__ << " " << __TIME__ << std::endl;
    
    std::cout << "C++ " << __cplusplus << std::endl;
}

int main(int argc, char **argv)
{
    appName = boost::filesystem::basename(argv[0]);
    
    std::string opt_inputDirectory;
    std::string opt_workDirectory;  // for downloads subdirectory
    bool flagVerbose = false;
    
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "print this message")
    ("version,v", "print the version information and exit")
    ("verbose", "be extra verbose") // Show errors and logs
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
    
    // Parse input files

    DEEPL::parseJSON(opt_workDirectory + "/downloads/drugshortage.json",
                     opt_workDirectory + "/output",
                     false);

#ifdef DEBUG
    std::clog << basename((char *)__FILE__) << ":" << __LINE__
    << ", toBeTranslatedSet size: " << DEEPL::toBeTranslatedSet.size()
    << std::endl;
#endif

    {
        std::string toBeTran = boost::algorithm::join(DEEPL::toBeTranslatedSet, "\n");
        std::ofstream outfile(opt_inputDirectory + "/deepl.drugshortage.in.txt");
        outfile << toBeTran;
        outfile << "\n";
        outfile.close();
    }
    
    return EXIT_SUCCESS;
}
