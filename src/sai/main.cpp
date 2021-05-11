//
//  main.cpp
//  sappinfo
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 11 May 2021
//
// Purpose:
// https://github.com/zdavatz/cpp2sqlite/issues/181

#include <iostream>
#include <string>
#include <set>
#include <unordered_set>
#include <libgen.h>     // for basename()

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string_regex.hpp>

#include <xlnt/xlnt.hpp>

#include "config.h"

namespace po = boost::program_options;
static std::string appName;

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

    std::clog << "OK" << std::endl;
    
    return EXIT_SUCCESS;
}
