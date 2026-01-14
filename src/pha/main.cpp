//
//  main.cpp
//  pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 10 May 2019

#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "refdata.hpp"
#include "swissmedic1.hpp"
#include "swissmedic2.hpp"
#include "bag.hpp"
#include "bagFHIR.hpp"
#include "report.hpp"
#include "config.h"
#include "ddd.hpp"
#include "aips.hpp"

namespace po = boost::program_options;
static std::string appName;

////////////////////////////////////////////////////////////////////////////

namespace PHARMA
{
}

////////////////////////////////////////////////////////////////////////////

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
    bool flagStorage = false;
    bool flagFHIR = false;

    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "print this message")
    ("version,v", "print the version information and exit")
    ("verbose", "be extra verbose") // Show errors and logs
    ("storage", "add one more column with medicine storage info")
    ("inDir", po::value<std::string>( &opt_inputDirectory )->required(), "input directory") //  without trailing '/'
    ("workDir", po::value<std::string>( &opt_workDirectory ), "parent of 'downloads' and 'output' directories, default as parent of inDir ")
    ("fhir", "Use BAG FHIR ndjson instead of BAG Preparation XML")
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
    
    if (vm.count("storage")) {
        flagStorage = true;
    }

    if (vm.count("fhir")) {
        flagFHIR = true;
    }

    ////////////////////////////////////////////////////////////////////////////

    std::string reportFilename("pharma_report.html");
    std::string reportTitle("Pharma Report");
    REP::init(opt_workDirectory + "/output/", reportFilename, reportTitle, false);

    REP::html_start_ul();
    for (int i=0; i<argc; i++)
        REP::html_li(argv[i]);
    
    REP::html_end_ul();

    REP::html_h1("File Analysis");

    // Read input files
    const std::string language("de");
    SWISSMEDIC1::parseXLXS(opt_workDirectory + "/downloads/swissmedic_packages.xlsx");
    SWISSMEDIC2::parseXLXS(opt_workDirectory + "/downloads/Erweiterte_Arzneimittelliste HAM.xlsx");
    if (flagFHIR) {
        BAGFHIR::parseNDJSON(opt_workDirectory + "/downloads/fhir-sl.ndjson", language, flagVerbose);
    } else {
        BAG::parseXML(opt_workDirectory + "/downloads/bag_preparations.xml", language, false);
    }

    REFDATA::parseXML(opt_workDirectory + "/downloads/refdata_pharma.xml", language);
    DDD::parseCSV(opt_inputDirectory + "/atc_ddd_2019.csv");

    if (flagStorage) {
        std::string type("fi"); // Fachinfo
        AIPS::parseXML(opt_workDirectory + "/downloads/aips.xml",
                       language,
                       type,
                       flagVerbose);
    }

    // Create CSV
    SWISSMEDIC1::createCSV(opt_workDirectory + "/output", flagStorage, flagFHIR);

    // Usage report
    REP::html_h1("Usage");
    SWISSMEDIC1::printUsageStats();
    BAG::printUsageStats();
    BAGFHIR::printUsageStats();
    REP::terminate();

    return EXIT_SUCCESS;
}
