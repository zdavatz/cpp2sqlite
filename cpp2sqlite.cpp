//
//  cpp2sqlite.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 15 Jan 2019
//

#include <iostream>
#include <sstream>
#include <string>
#include <sqlite3.h>
#include <libgen.h>     // for basename()

#include <boost/program_options.hpp>
#include <exception>

#include "aips.hpp"
#include "sqlDatabase.hpp"

namespace po = boost::program_options;

void on_version()
{
    std::cout << "C++ " << __cplusplus << std::endl;
    std::cout << "SQLITE_VERSION: " << SQLITE_VERSION << std::endl;
    std::cout << "BOOST_VERSION: " << BOOST_LIB_VERSION << std::endl;
}

int main(int argc, char **argv)
{
    std::string xmlFilename;
    std::string language;

    // See file Aips2Sqlite.java, function commandLineParse(), line 71, line 179
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "print this message")
        ("version,v", "print the version information and exit")
        ("verbose", "be extra verbose") // Show errors and logs
        ("nodown", "no download, parse only")
        ("lang", po::value<std::string>( &language )->required(), "use given language (de/fr)")
        ("alpha", "only include titles which start with option value")  // Med title
        ("regnr", "only include medications which start with option value") // Med regnr
        ("owner", "only include medications owned by option value") // Med owner
        ("pseudo", "adds pseudo expert infos to db") // Pseudo fi
        ("inter", "adds drug interactions to db")
        ("pinfo", "generate patient info htmls") // Generate pi
        ("xml", po::value<std::string>( &xmlFilename )->required(), "generate xml file")
        ("gln", "generate csv file with Swiss gln codes") // Shopping cart
        ("shop", "generate encrypted files for shopping cart")
        ("onlyshop", "skip generation of sqlite database")
        ("zurrose", "generate zur Rose full article database or stock/like files (fulldb/atcdb/quick)") // Zur Rose DB
        ("desitin", "generate encrypted files for Desitin")
        ("onlydesitin", "skip generation of sqlite database") // Only Desitin DB
        ("takeda", po::value<float>(), "generate sap/gln matching file")
        ("dailydrugcosts", "calculates the daily drug costs")
        ("smsequence", "generates swissmedic sequence csv")
        ("packageparse", "extract dosage information from package name")
        ("zip", "generate zipped big files (sqlite or xml)")
        ("reports", "generates various reports")
        ("indications", "generates indications section keywords report")
        ("plain", "does not update the package section")
        ("test", "starts in test mode")
        ("stats", po::value<float>(), "generates statistics for given user")
        ;
    
    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm); // populate vm
        
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }
        
        if (vm.count("version")) {
            std::cout << basename(argv[0]) << " " << __DATE__ << " " << __TIME__ << std::endl;
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
        std::cout << "TODO: show errors, show logs" << std::endl;
    }

    AIPS::MedicineList &list = AIPS::parseXML(xmlFilename);
    std::cout << "title count: " << list.size() << std::endl;

    sqlite3 *db = AIPS::createDB("amiko_db_full_idx_de.db");

    sqlite3_stmt *statement;
    AIPS::prepareStatement("amikodb", &statement,
                           "null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?");

    std::cerr << "Populating amiko_db_full_idx_de.db" << std::endl;
    for (AIPS::Medicine m : list) {
        AIPS::bindText("amikodb", statement, 1, m.title);
        AIPS::bindText("amikodb", statement, 2, m.auth);
        AIPS::bindText("amikodb", statement, 4, m.subst);
        // TODO: add all other columns

        AIPS::runStatement("amikodb", statement);
    }

    AIPS::destroyStatement(statement);

    int rc = sqlite3_close(db);
    if (rc != SQLITE_OK)
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", rc" << rc << std::endl;

    return EXIT_SUCCESS;
}
