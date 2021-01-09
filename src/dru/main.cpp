//
//  main.cpp
//  sappinfo
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 9 Jan 2021
//
// https://github.com/zdavatz/cpp2sqlite/issues/140

#include <iostream>
#include <string>
#include <set>
#include <unordered_set>
#include <sqlite3.h>
#include <libgen.h>     // for basename()

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string_regex.hpp>

#include <xlnt/xlnt.hpp>

#include "config.h"

#define FIRST_DATA_ROW_INDEX    1

namespace po = boost::program_options;
static std::string appName;

#pragma mark -

void on_version()
{
    std::cout << appName << " " << PROJECT_VER
    << ", " << __DATE__ << " " << __TIME__ << std::endl;

    std::cout << "SQLITE_VERSION: " << SQLITE_VERSION << std::endl;
    std::cout << "C++ " << __cplusplus << std::endl;
}

int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
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
    ("inDir", po::value<std::string>( &opt_inputDirectory )->required(), "input directory") //  without trailing '/'
    ("workDir", po::value<std::string>( &opt_workDirectory ), "parent of 'downloads' and 'output' directories, default as parent of inDir ")
    ("lang", po::value<std::string>( &opt_language )->default_value("de"), "use given language (de/fr)")
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

    std::string dbFilename = opt_workDirectory + "/output/amiko_db_full_idx_" + opt_language + ".db";
    sqlite3 *db;
    int rc = sqlite3_open(dbFilename.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << std::endl;
        
        return 1;
    }
    
    char *errmsg;
    sqlite3_exec(db, "select * from amikodb where title LIKE 'ponstan%';", callback, NULL, &errmsg);
    sqlite3_close(db);
    
    return EXIT_SUCCESS;
}
