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

#include <sqlite3.h>
#include "sqlDatabase.hpp"

#include "config.h"
#include "sai.hpp"

constexpr std::string_view TABLE_NAME_SAI = "sai_db";

namespace po = boost::program_options;
static std::string appName;

static DB::Sql sqlDb;

void on_version()
{
    std::cout << appName << " " << PROJECT_VER
    << ", " << __DATE__ << " " << __TIME__ << std::endl;
    
    std::cout << "C++ " << __cplusplus << std::endl;
}

void openDB(const std::string &filename)
{
    std::clog << std::endl << "Create DB: " << filename << std::endl;

    sqlDb.openDB(filename);
    sqlDb.createTable(TABLE_NAME_SAI, "_id INTEGER PRIMARY KEY AUTOINCREMENT, approval_number TEXT, sequence_number TEXT, package_code TEXT, approval_status TEXT, note_free_text TEXT, package_size TEXT, package_unit TEXT, revocation_waiver_date TEXT, btm_code TEXT, gtin_industry TEXT, in_trade_date_industry TEXT, out_of_trade_date_industry TEXT, description_en_refdata TEXT, description_fr_refdata TEXT");
    sqlDb.createIndex(TABLE_NAME_SAI, "idx_", {"approval_number", "sequence_number", "package_code"});
    sqlDb.prepareStatement(TABLE_NAME_SAI,
                           "null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?");
}

void closeDB()
{
    sqlDb.destroyStatement();
    sqlDb.closeDB();
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

    SAI::parseXML(opt_workDirectory + "/downloads/Typ1/Typ1-Packungen.XML");

    std::string dbFilename = opt_workDirectory + "/output/sai.db";
    openDB(dbFilename);

    int i = 0;
    auto packages = SAI::getPackages();
    int total = packages.size();
    for (auto package : SAI::getPackages()) {
        std::cerr << "\r" << 100*i/total << " % ";
        sqlDb.bindText(1, package.approvalNumber);
        sqlDb.bindText(2, package.sequenceNumber);
        sqlDb.bindText(3, package.packageCode);
        sqlDb.bindText(4, package.approvalStatus);
        sqlDb.bindText(5, package.noteFreeText);
        sqlDb.bindText(6, package.packageSize);
        sqlDb.bindText(7, package.packageUnit);
        sqlDb.bindText(8, package.revocationWaiverDate);
        sqlDb.bindText(9, package.btmCode);
        sqlDb.bindText(10, package.gtinIndustry);
        sqlDb.bindText(11, package.inTradeDateIndustry);
        sqlDb.bindText(12, package.outOfTradeDateIndustry);
        sqlDb.bindText(13, package.descriptionEnRefdata);
        sqlDb.bindText(14, package.descriptionFrRefdata);
        sqlDb.runStatement(TABLE_NAME_SAI);
        i++;
    }
    std::cerr << "\r" << "100 % ";

    closeDB();
    
    return EXIT_SUCCESS;
}
