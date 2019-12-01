//
//  main.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 29 Oct 2019

#include <iostream>
#include <vector>
#include <map>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <libgen.h>     // for basename()

#include "config.h"
#include "sqlDatabase.hpp"
#include "Article.hpp"
#include "atc.hpp"
#include "stamm.hpp"
#include "voll.hpp"
#include "kunden.hpp"
#include "generika.hpp"
#include "direkt.hpp"
#include "nota.hpp"
#include "report.hpp"
#include "bag.hpp"
#include "galen.hpp"
#include "swissmedic.hpp"
#include "neu.hpp"

namespace po = boost::program_options;

static std::string appName;

#pragma mark -

#ifdef OBSOLETE_STUFF
// See file DispoParse.java line 861
void getAtcMap(const std::string downloadDir)
{
    // TODO: parse './downloads/epha_atc_codes_csv.csv'
}
#endif

// See file DispoParse.java line 725
void getSLMap(const std::string downloadDir,
              const std::string &language,
              bool verbose)
{
    BAG::parseXML(downloadDir + "/bag_preparations.xml", language, false);
}

// See file DispoParse.java line 818
void enhanceFlags(const std::string downloadDir)
{
    // TODO: parse './downloads/swissmedic_packages.xlsx'
}

#ifdef OBSOLETE_STUFF
// See file DispoParse.java line 312
void processLikes(const std::string inDir)
{
    // TODO: parse './input/zurrose/like_db_zurrose.csv'
}
#endif

// See file DispoParse.java line 363
// fulldb,atcdb
void generateFullSQLiteDB(const std::string inDir, const std::string type)
{
    VOLL::parseCSV(inDir + "/zurrose/artikel_vollstamm_zurrose.csv", type);
    VOLL::createDB();
}

// See file DispoParse.java line 582
// quick
void generatePharmaToStockCsv(std::string inDir, std::string outDir)
{
    STAMM::parseCSV(inDir + "/zurrose/artikel_stamm_zurrose.csv");
    STAMM::parseVoigtCSV(inDir + "/zurrose/artikel_stamm_voigt.csv");
    
    STAMM::createStockCSV(outDir + "/rose_stock.csv");
}

// See ShoppingCartRose.java 54 encryptCustomerMapToFile()
void generateKunden(std::string inDir, std::string outDir)
{
    KUNDEN::parseCSV(inDir + "/zurrose/Kunden_alle.csv");

    KUNDEN::createConditionsJSON(outDir + "/rose_conditions.json");
    KUNDEN::createIdsJSON(outDir + "/rose_ids.json");
}

void generateKundenNeu(std::string inDir, std::string outDir)
{
    NEU::parseCSV(inDir + "/zurrose/Kunden_alle_NEU.csv");
    NEU::createConditionsNewJSON(outDir + "/rose_conditions_new.json");
}

// See ShoppingCartRose.java 260 encryptAutoGenerikaToFile()
void generateAutogenerika(std::string inDir, std::string outDir)
{
    GENERIKA::parseCSV(inDir + "/zurrose/Autogenerika.csv");
    GENERIKA::createJSON(outDir + "/rose_autogenerika.json");
}

// See ShoppingCartRose.java 303 encryptDirectSubstToFile()
void generateDirect(std::string inDir, std::string outDir)
{
    DIREKT::parseCSV(inDir + "/zurrose/direct_subst_zurrose.csv");
    DIREKT::createJSON(outDir + "/output/rose_direct_subst.json");
}

// See ShoppingCartRose.java 339 encryptNotaToFile()
void generateNota(std::string inDir, std::string outDir)
{
    NOTA::parseCSV(inDir + "/zurrose/nota_zurrose.csv");
    NOTA::createJSON(outDir + "/output/rose_nota.json");
}

void on_version()
{
    std::cout << appName << " " << PROJECT_VER
    << ", " << __DATE__ << " " << __TIME__ << std::endl;
    
    std::cout << "C++ " << __cplusplus << std::endl;
}

int main(int argc, char **argv)
{
    //appName = boost::filesystem::basename(argv[0]);
    appName = basename(argv[0]);

    std::string opt_inputDirectory;
    std::string opt_workDirectory;  // for downloads subdirectory
    std::string opt_language;
    std::string opt_zurrose;
    bool flagVerbose = false;
    
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "print this message")
    ("version,v", "print the version information and exit")
    ("verbose", "be extra verbose") // Show errors and logs
    ("lang", po::value<std::string>( &opt_language )->default_value("de"), "use given language (de/fr)")
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
    
    // Report
    std::string reportFilename("zurrose_report_" + opt_language + ".html");
    std::string language = opt_language;
    boost::to_upper(language);
    //ofs2 << "<title>" << title << " Report " << language << "</title>";
    std::string reportTitle("Zurrose Report " + language);
    REP::init(opt_workDirectory + "/output/", reportFilename, reportTitle, flagVerbose);
    REP::html_start_ul();
    for (int i=0; i<argc; i++)
        REP::html_li(argv[i]);

    REP::html_end_ul();
    
    // TODO: create index with links to expected h1 titles
    REP::html_h1("File Analysis");
    
    // Parse input files
    // For French names of medicines
    ATC::parseTXT(opt_inputDirectory + "/atc_codes_multi_lingual.txt", opt_language, flagVerbose);
    
    // See ShoppingCartRose.java line 24 encryptFiles()
#ifdef OBSOLETE_STUFF
    // TODO: parse 'Abverkaufszahlen.csv' to generate 'rose_sales_fig.ser'
#endif

    generateKunden(opt_inputDirectory, opt_workDirectory + "/output");
    generateKundenNeu(opt_inputDirectory, opt_workDirectory + "/output");
    generateAutogenerika(opt_inputDirectory, opt_workDirectory + "/output");
    generateDirect(opt_inputDirectory, opt_workDirectory);
    generateNota(opt_inputDirectory, opt_workDirectory);

    if ((opt_zurrose == "fulldb") || (opt_zurrose == "atcdb"))
    {
        // See .java initSqliteDB()
        std::string dbName = (opt_zurrose == "fulldb") ? "rose_db_new_full.db" : "rose_db_new_atc_only.db";
        VOLL::openDB(opt_workDirectory + "/output/" + dbName);

#ifdef OBSOLETE_STUFF
        getAtcMap(opt_workDirectory + "/downloads");
#endif

        // See file DispoParse.java line 725
        getSLMap(opt_workDirectory + "/downloads", language, false);

        // Enhance SL map with information on"Abgabekategorie"
        enhanceFlags(opt_workDirectory + "/downloads");

        // Get galenic form to galenic code map. See file DispoParse.java line 277 getGalenicCodeMap()
        GALEN::parseTXT(opt_inputDirectory);

#ifdef OBSOLETE_STUFF
        processLikes(opt_inputDirectory);
#endif

        SWISSMEDIC::parseXLXS(opt_workDirectory + "/downloads");

        // Process CSV file and generate Sqlite DB
        generateFullSQLiteDB(opt_inputDirectory, opt_zurrose);
    }
    else if (opt_zurrose == "quick") {
        generatePharmaToStockCsv(opt_inputDirectory, opt_workDirectory + "/output");
    }
    
    if ((opt_zurrose == "fulldb") ||
        (opt_zurrose == "atcdb"))
    {
        VOLL::closeDB();
    }

    REP::terminate();

    return EXIT_SUCCESS;
}
