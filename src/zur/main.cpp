//
//  main.cpp
//  sappinfo
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 29 Oct 2019

#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string_regex.hpp>

#include <libgen.h>     // for basename()

#include "config.h"

namespace po = boost::program_options;
static std::string appName;

#pragma mark - ZUR

namespace ZUR
{
void parseCSV(const std::string &filename)
{
    std::clog << std::endl << "Reading artikel_stamm_zurrose CSV" << std::endl;
    
    try {
        //std::clog << std::endl << "Reading CSV" << std::endl;
        std::ifstream file(filename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str)) {
            
            if (header) {
                header = false;

#ifdef DEBUG
                std::vector<std::string> headerTitles;
                boost::algorithm::split(headerTitles, str, boost::is_any_of(";"));
                auto colLetter = 'A';
                for (auto t : headerTitles)
                    std::clog << colLetter++ << "\t" << t << std::endl;
#endif
                continue;
            }
        }
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
}
}

#pragma mark -

// See file DispoParse.java line 861
void getAtcMap()
{
    // TODO: parse  './downloads/epha_atc_codes_csv.csv'
}

// See file DispoParse.java line 725
void getSLMap()
{
    // TODO: parse  './downloads/bag_preparations_xml.xml'
}

// See file DispoParse.java line 818
void enhanceFlags()
{
    // TODO: parse './downloads/swissmedic_packages_xlsx.xlsx'
}

// See file DispoParse.java line 277
void getGalenicCodeMap()
{
    // TODO: parse './input/zurrose/galenic_codes_map_zurrose.csv'
}

#ifdef OBSOLETE_STUFF
// See file DispoParse.java line 312
void processLikes()
{
    // TODO: parse './input/zurrose/like_db_zurrose.csv'
}
#endif

// See file DispoParse.java line 363
void generateFullSQLiteDB(std::string type)
{
    // TODO: parse 'input/zurrose/artikel_vollstamm_zurrose.csv'
}

// See file DispoParse.java line 582
void generatePharmaToStockCsv(std::string dir)
{
    ZUR::parseCSV(dir + "/zurrose/artikel_stamm_zurrose.csv");
}

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
    std::string opt_zurrose;
    bool flagVerbose = false;
    
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "print this message")
    ("version,v", "print the version information and exit")
    ("verbose", "be extra verbose") // Show errors and logs
    ("zurrose", po::value<std::string>( &opt_zurrose )->default_value("fulldb"), "generate zur Rose full article database or stock/like files (fulldb/atcdb/quick)")
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
    
    // See ShoppingCartRose.java line 24 encryptFiles()
#if 0 // we don't need this
    // TODO: parse 'Abverkaufszahlen.csv' to generate 'rose_sales_fig.ser'
#endif
    // TODO: parse 'Kunden_alle.csv' to generate 'Kunden_alle.ser' and 'rose_ids.ser'
    // TODO: parse 'Autogenerika.csv' to generate 'rose_autogenerika.ser'
    // TODO: parse 'direct_subst_zurrose.csv' to generate 'rose_direct_subst.ser'
    // TODO: parse 'nota_zurrose.csv' to generate 'rose_nota.ser'

    if (opt_zurrose == "fulldb") {
        // TODO: initSqliteDB("rose_db_new_full.db");
    }
    else if (opt_zurrose == "atcdb") {
        // TODO: initSqliteDB("rose_db_new_atc_only.db");
    }

    if ((opt_zurrose == "fulldb") || (opt_zurrose == "atcdb"))
    {
        // Process atc map
        getAtcMap();

        // Get SL map
        getSLMap();

        // Enhance SL map with information on"Abgabekategorie"
        enhanceFlags();

        // Get galenic form to galenic code map
        getGalenicCodeMap();

#ifdef OBSOLETE_STUFF
        processLikes();
#endif

        // Process CSV file and generate Sqlite DB
        generateFullSQLiteDB(opt_zurrose);
    }
    else if (opt_zurrose == "quick") {
        generatePharmaToStockCsv(opt_inputDirectory); // Generate GLN to stock map (csv file)
    }

    return EXIT_SUCCESS;
}
