//
//  main.cpp
//  nonpharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by B123400 on 30 Sep 2023

#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <xlnt/xlnt.hpp>
#include <sqlite3.h>
#include <iomanip>
#include <sstream>
#include "sqlDatabase.hpp"
#include "report.hpp"

#include "transfer.hpp"

#include "config.h"

#define COLUMN_A        0   // GTIN des Artikels
#define COLUMN_B        1   // GLN
#define COLUMN_C        2   // Zielmarkt
#define COLUMN_D        3   // GPC Brick
#define COLUMN_E        4   // Artikelbeschreibung / Sprache - da
#define COLUMN_F        5   // Artikelbeschreibung / Sprache - de
#define COLUMN_G        6   // Artikelbeschreibung / Sprache - en
#define COLUMN_H        7   // Artikelbeschreibung / Sprache - fr
#define COLUMN_I        8   // Artikelbeschreibung / Sprache - it
#define COLUMN_J        9   // Hersteller: Name
#define COLUMN_K        10  // Verfügbarkeit: Startdatum
#define COLUMN_L        11  // Bruttogewicht
#define COLUMN_M        12  // Nettogewicht
#define COLUMN_N        13  // Verknüpfungsangaben zu externen Dateien - 0 - URI
#define COLUMN_O        14  // Verknüpfungsangaben zu externen Dateien - 1 - URI
#define COLUMN_P        15  // Verpackung - 0 - Enthaltene Menge

#define FIRST_DATA_ROW_INDEX    2

constexpr std::string_view TABLE_NAME_NONPHARMA = "nonpharma_db";

namespace po = boost::program_options;
static std::string appName;
static DB::Sql sqlDb;

std::vector<std::vector<std::string>> xlsxRows;
std::set<std::string> xlsxGtins;

////////////////////////////////////////////////////////////////////////////

namespace NONPHARMA
{
}

////////////////////////////////////////////////////////////////////////////

void on_version()
{
    std::cout << appName << " " << PROJECT_VER
    << ", " << __DATE__ << " " << __TIME__ << std::endl;
    
    std::cout << "C++ " << __cplusplus << std::endl;
}

void parseXLXS(const std::string &inFilename,
               bool verbose)
{
    std::clog << std::endl << "Reading " << inFilename << std::endl;
    xlnt::workbook wb;
    wb.load(inFilename);
    
    auto ws = wb.active_sheet();

    int skipHeaderCount = 0;
    for (auto row : ws.rows(false)) {
        if (++skipHeaderCount <= FIRST_DATA_ROW_INDEX) {
            continue;
        }

        std::vector<std::string> aSingleRow;
        int i = 0;
        for (auto cell : row) {
            if (i > COLUMN_N) {
                break; // The last column
            }
            aSingleRow.push_back(cell.to_string());
            i++;
        }

        xlsxRows.push_back(aSingleRow);
    }
}

void openDB(const std::string &filename)
{
    std::clog << std::endl << "Create DB: " << filename << std::endl;

    sqlDb.openDB(filename);
    sqlDb.createTable(TABLE_NAME_NONPHARMA, "_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "gtin TEXT, "
        "gln TEXT, "
        "target_market TEXT, "
        "gpc TEXT, "
        "trade_item_description_da TEXT, "
        "trade_item_description_de TEXT, "
        "trade_item_description_en TEXT, "
        "trade_item_description_fr TEXT, "
        "trade_item_description_it TEXT, "
        "manufacturer_name TEXT, "
        "start_availability_date TEXT, "
        "gross_weight TEXT, "
        "netWeight TEXT, "
        "referenced_collection_list_uniform_resource_identifier TEXT, "
        "packages_contained_amount TEXT, "
        "price TEXT, "
        "pub_price TEXT "
    );
    sqlDb.prepareStatement(TABLE_NAME_NONPHARMA,
                           "null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?");
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
    
    // ////////////////////////////////////////////////////////////////////////////

    std::string reportFilename("nonpharma_report.html");
    std::string reportTitle("NONPHARMA Report");
    REP::init(opt_workDirectory + "/output/", reportFilename, reportTitle, false);

    REP::html_start_ul();
    for (int i=0; i<argc; i++)
        REP::html_li(argv[i]);
    
    REP::html_end_ul();

    parseXLXS(opt_workDirectory + "/downloads/nonpharma.xlsx", false);
    TRANSFER::parseDAT(opt_workDirectory + "/downloads/transfer/transfer.dat");

    std::string dbFilename = opt_workDirectory + "/output/nonpharma.db";
    openDB(dbFilename);

    int transferDatHitCount = 0;
    int i = 0;
    int total = xlsxRows.size();
    for (auto row : xlsxRows) {
        std::cerr << "\r" << 100*i/total << " % ";
        int cellIndex = 1;
        int sqlIndex = 1;
        for (auto cell : row) {
            if (cellIndex != 15) {
                sqlDb.bindText(sqlIndex, cell);
                sqlIndex++;
            }
            cellIndex++;
        }
        std::string gtin = row[COLUMN_A];
        TRANSFER::Entry entry = TRANSFER::getEntryWithGtin(gtin);
        if (!entry.ean13.empty()) {
            transferDatHitCount++;
            std::stringstream stream1;
            stream1 << std::fixed << std::setprecision(2) << entry.price;
            sqlDb.bindText(sqlIndex++, stream1.str());

            std::stringstream stream2;
            stream2 << std::fixed << std::setprecision(2) << entry.pub_price;
            sqlDb.bindText(sqlIndex++, stream2.str());
        } else {
            sqlDb.bindText(sqlIndex++, "");
            sqlDb.bindText(sqlIndex++, "");
        }
        sqlDb.runStatement(TABLE_NAME_NONPHARMA);
        xlsxGtins.insert(gtin);
        i++;
    }

    i = 0;
    total = TRANSFER::getEntries().size();
    for (auto pair : TRANSFER::getEntries()) {
        auto entry = pair.second;
        // #240 add Products from transfer.dat to nonpharma.db
        if (!boost::starts_with(entry.ean13, "7680") && xlsxGtins.find(entry.ean13) == xlsxGtins.end()) {
            sqlDb.bindText(1, entry.ean13); // "gtin TEXT, "
            sqlDb.bindText(2, ""); // "gln TEXT, "
            sqlDb.bindText(3, ""); // "target_market TEXT, "
            sqlDb.bindText(4, ""); // "gpc TEXT, "
            sqlDb.bindText(5, entry.description); // "trade_item_description_da TEXT, "
            sqlDb.bindText(6, entry.description); // "trade_item_description_de TEXT, "
            sqlDb.bindText(7, ""); // "trade_item_description_en TEXT, "
            sqlDb.bindText(8, ""); // "trade_item_description_fr TEXT, "
            sqlDb.bindText(9, ""); // "trade_item_description_it TEXT, "
            sqlDb.bindText(10, ""); // "manufacturer_name TEXT, "
            sqlDb.bindText(11, ""); // "start_availability_date TEXT, "
            sqlDb.bindText(12, ""); // "gross_weight TEXT, "
            sqlDb.bindText(13, ""); // "netWeight TEXT, "
            sqlDb.bindText(14, ""); // "referenced_collection_list_uniform_resource_identifier TEXT, "
            sqlDb.bindText(15, ""); // "packages_contained_amount TEXT, "

            std::stringstream stream1;
            stream1 << std::fixed << std::setprecision(2) << entry.price;
            sqlDb.bindText(16, stream1.str()); // "price TEXT, "

            std::stringstream stream2;
            stream2 << std::fixed << std::setprecision(2) << entry.pub_price;
            sqlDb.bindText(17, stream2.str()); // "pub_price TEXT "
            sqlDb.runStatement(TABLE_NAME_NONPHARMA);
        }

        std::cerr << "\rAdding from transfer.dat " << 100*i/total << " % ";
        i++;
    }

    closeDB();

    std::cerr << "\r" << "100 % ";

    REP::html_h2("Stats: ");
    REP::html_start_ul();
    REP::html_li("Row count: " + std::to_string(total));
    REP::html_li("Prices via transfer.dat count: " + std::to_string(transferDatHitCount));
    REP::html_end_ul();

    return EXIT_SUCCESS;
}
