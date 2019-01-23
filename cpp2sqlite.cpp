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
#include <exception>

#include <sqlite3.h>
#include <libgen.h>     // for basename()

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "aips.hpp"
#include "refdata.hpp"
#include "swissmedic.hpp"
#include "bag.hpp"

#include "sqlDatabase.hpp"

namespace po = boost::program_options;

void on_version()
{
    std::cout << "C++ " << __cplusplus << std::endl;
    std::cout << "SQLITE_VERSION: " << SQLITE_VERSION << std::endl;
    std::cout << "BOOST_VERSION: " << BOOST_LIB_VERSION << std::endl;
}

int countAipsPackagesInSwissmedic(AIPS::MedicineList &list)
{
    int count = 0;
    for (AIPS::Medicine m : list) {
        std::vector<std::string> regnrs;
        boost::algorithm::split(regnrs, m.regnrs, boost::is_any_of(", "), boost::token_compress_on);
        for (auto rn : regnrs) {
            count += SWISSMEDIC::countRowsWithRn(rn);
        }
    }
    return count;
}

int main(int argc, char **argv)
{
    std::string appName = boost::filesystem::basename(argv[0]);

    std::string opt_downloadDirectory;
    std::string opt_language;
    bool flagXml = false;
    bool flagVerbose = false;
    //bool flagPinfo = false;
    std::string type("fi"); // Fachinfo
    std::string opt_aplha;
    std::string opt_regnr;
    std::string opt_owner;

    // See file Aips2Sqlite.java, function commandLineParse(), line 71, line 179
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "print this message")
        ("version,v", "print the version information and exit")
        ("verbose", "be extra verbose") // Show errors and logs
        ("nodown", "no download, parse only")
        ("lang", po::value<std::string>( &opt_language )->default_value("de"), "use given language (de/fr)")
        ("alpha", po::value<std::string>( &opt_aplha ), "only include titles which start with arg value")  // Med title
        ("regnr", po::value<std::string>( &opt_regnr ), "only include medications which start with arg value") // Med regnr
        ("owner", po::value<std::string>( &opt_owner ), "only include medications owned by arg value") // Med owner
        ("pseudo", "adds pseudo expert infos to db") // Pseudo fi
        ("inter", "adds drug interactions to db")
        ("pinfo", "generate patient info htmls") // Generate pi
        ("xml", "generate xml file")
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
        ("inDir", po::value<std::string>( &opt_downloadDirectory )->required(), "download directory (without trailing /)")
        ;
    
    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm); // populate vm
        
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }
        
        if (vm.count("version")) {
            std::cout << appName << " " << __DATE__ << " " << __TIME__ << std::endl;
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
    
    if (vm.count("xml")) {
        flagXml = true;
        std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << " flagXml: " << flagXml << std::endl;
    }
    
    if (vm.count("pinfo")) {
        //flagPinfo = true;
        type = "pi";
        //std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << " flagPinfo: " << flagPinfo << std::endl;
    }

    AIPS::MedicineList &list = AIPS::parseXML(opt_downloadDirectory + "/aips_xml.xml", opt_language, type);
    
    REFDATA::parseXML(opt_downloadDirectory + "/refdata_pharma_xml.xml", opt_language);
    
    SWISSMEDIC::parseXLXS(opt_downloadDirectory + "/swissmedic_packages_xlsx.xlsx");
    
    BAG::parseXML(opt_downloadDirectory + "/bag_preparations_xml.xml");

    if (flagXml) {
        std::cerr << "Creating XML not yet implemented" << std::endl;
    }
    else {
        std::string dbFilename = "amiko_db_full_idx_" + opt_language + ".db";
        sqlite3 *db = AIPS::createDB(dbFilename);

        sqlite3_stmt *statement;
        AIPS::prepareStatement("amikodb", &statement,
                               "null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?");

        std::cerr << "Populating " << dbFilename << std::endl;
        int statsFoundRefdataCount = 0;
        int statsNotFoundRefdataCount = 0;
        int statsFoundSwissmedicCount = 0;
        int statsNotFoundAnywhereCount = 0;

        for (AIPS::Medicine m : list) {
            // See DispoParse.java:164 addArticleDB()
            // See SqlDatabase.java:347 addExpertDB()
            AIPS::bindText("amikodb", statement, 1, m.title);
            AIPS::bindText("amikodb", statement, 2, m.auth);
            AIPS::bindText("amikodb", statement, 3, m.atc);
            AIPS::bindText("amikodb", statement, 4, m.subst);
            AIPS::bindText("amikodb", statement, 5, m.regnrs);
            
#if 1 // pack_info_str
            // For each regnr in the vector add the name(s) from refdata
            std::vector<std::string> regnrs;
            boost::algorithm::split(regnrs, m.regnrs, boost::is_any_of(", "), boost::token_compress_on);
            //std::cerr << basename((char *)__FILE__) << ":" << __LINE__  << "regnrs size: " << regnrs.size() << std::endl;
            std::string packInfo;
            int i=0;
            for (auto rn : regnrs) {
                //std::cerr << basename((char *)__FILE__) << ":" << __LINE__  << " rn: " << rn << std::endl;
                std::string name = REFDATA::getNames(rn);
                if (!name.empty()) {
                    if (i>0)
                        packInfo += "\n";

                    packInfo += name;
                    i++;
                    statsFoundRefdataCount++;
                }
                else {
                    //std::clog << basename((char *)__FILE__) << ":" << __LINE__ << " rn: " << rn << " NOT FOUND in refdata" << std::endl;
                    statsNotFoundRefdataCount++;

                    // Search in swissmedic
                    name = SWISSMEDIC::getNames(rn);
                    if (!name.empty()) {
                        if (i>0)
                            packInfo += "\n";

                        packInfo += name;
                        i++;
                        statsFoundSwissmedicCount++;
                    }
                    else {
                        statsNotFoundAnywhereCount++;
                        if (flagVerbose)
                            std::clog << "\trn: " << rn << " not found" << std::endl;
                    }
                }
                
                if (!name.empty()) {
                    std::string flags = BAG::getFlags(rn);
                    if (!flags.empty())
                        packInfo += flags;
                }
            } // for

            if (!packInfo.empty())
                AIPS::bindText("amikodb", statement, 11, packInfo);
#endif

            // TODO: add all other columns

            AIPS::runStatement("amikodb", statement);
        }
        
        std::cerr
        << "REGNRS in refdata: " << statsFoundRefdataCount
        << ", not in refdata: " << statsNotFoundRefdataCount
        << ", in swissmedic: " << statsFoundSwissmedicCount
        << ", not found anywhere: " << statsNotFoundAnywhereCount
        << std::endl;
        
        if ((statsNotFoundAnywhereCount > 0) && !flagVerbose)
            std::cerr << "Run with --verbose to see REGNRS not found" << std::endl;

        AIPS::destroyStatement(statement);

        int rc = sqlite3_close(db);
        if (rc != SQLITE_OK)
            std::cerr << basename((char *)__FILE__) << ":" << __LINE__ << ", rc" << rc << std::endl;
    }
    
    std::cerr << "Swissmedic has " << countAipsPackagesInSwissmedic(list) << " packages matching AIPS" << std::endl;

    return EXIT_SUCCESS;
}
