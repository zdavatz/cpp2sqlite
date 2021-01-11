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
#include <nlohmann/json.hpp>
#include <xlnt/xlnt.hpp>

#include "config.h"

namespace po = boost::program_options;
static std::string appName;
std::string opt_language;
sqlite3 *db;

#pragma mark -

void on_version()
{
    std::cout << appName << " " << PROJECT_VER
    << ", " << __DATE__ << " " << __TIME__ << std::endl;

    std::cout << "SQLITE_VERSION: " << SQLITE_VERSION << std::endl;
    std::cout << "C++ " << __cplusplus << std::endl;
}

static std::map<int64_t, nlohmann::json> drugshortageJsonMap;

nlohmann::json jsonEntryForGtin(std::string gtinStr) {
    for (nlohmann::json::iterator it = drugshortageJson.begin(); it != drugshortageJson.end(); ++it) {
        auto entry = it.value();
        int64_t thisGtin = entry["gtin"].get<int64_t>();
        if (std::to_string(thisGtin) == gtinStr) {
            return entry;
        }
    }
    return nlohmann::json::object();
}

int onProcessRow(void *NotUsed, int argc, char **argv, char **azColName){
    std::string packageStr = std::string(argv[17]);
    std::cout << "packageStr: " << packageStr << std::endl;
    std::vector<std::string> lines;
    boost::algorithm::split(lines, packageStr, boost::is_any_of("\n"), boost::token_compress_on);
    for (std::string line : lines){
        std::vector<std::string> parts;
        boost::algorithm::split(parts, line, boost::is_any_of("|"), boost::token_compress_on);
        std::string gtinString = parts[6];
        std::cout << "gtin: " << gtinString << std::endl;
        
        auto result = jsonEntryForGtin(gtinString);
        std::cout << "found json entry: " << result << std::endl;
    }
    return 0;
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
    
    int rc = sqlite3_open(dbFilename.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << std::endl;
        
        return 1;
    }
    char *errmsg;

    nlohmann::json drugshortageJson;
    std::string jsonFilename = opt_inputDirectory + "/drugshortage.json";
    std::ifstream jsonInputStream(jsonFilename);
    jsonInputStream >> drugshortageJson;
    for (nlohmann::json::iterator it = drugshortageJson.begin(); it != drugshortageJson.end(); ++it) {
        auto entry = it.value();
        int64_t thisGtin = entry["gtin"].get<int64_t>();
        drugshortageJsonMap[thisGtin] = entry;
        sqlite3_exec(db, ("select * from amikodb where packages LIKE '%|" + std::to_string(thisGtin) + "|%';").c_str(), onProcessRow, (void *)thisGtin, &errmsg);
    }

    sqlite3_close(db);
    
    return EXIT_SUCCESS;
}
