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
#include "kunden.hpp"
#include "generika.hpp"
#include "direkt.hpp"
#include "nota.hpp"

#define WITH_PROGRESS_BAR
static constexpr std::string_view TABLE_NAME_ROSE = "rosedb";
static constexpr std::string_view TABLE_NAME_ANDROID = "android_metadata";
static constexpr std::string_view NO_DETAILS = "k.A.";    // keine Angaben

namespace po = boost::program_options;

static std::string appName;

static DB::Sql sqlDb;
std::vector<Article> articles;

#pragma mark - CSV

namespace CSV
{
constexpr std::string_view CSV_SEPARATOR = ";";

// See DispoParse.java line 363 generateFullSQLiteDB()
void parseVollstamm(const std::string &filename, const std::string type)
{
    std::clog << std::endl << "Reading artikel_vollstamm_zurrose CSV" << std::endl;

#ifdef DEBUG
    std::clog  << "Filename: " << filename << std::endl;
#endif
    
    try {
        std::ifstream file(filename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str)) {
            
            if (header) {
                header = false;

#ifdef DEBUG
                std::vector<std::string> headerTitles;
                boost::algorithm::split(headerTitles, str, boost::is_any_of(CSV_SEPARATOR));
                std::clog << "Number of columns: " << headerTitles.size() << std::endl;
                auto colLetter = 'A';
                for (auto t : headerTitles)
                    std::clog << colLetter++ << "\t" << t << std::endl;
#endif
                continue;
            }
            
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(CSV_SEPARATOR));
            
            if (columnVector.size() != 21) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }

            Article a;
            a.pharma_code = columnVector[0]; // A
            a.pack_title = columnVector[1];
            
            a.ean_code = columnVector[2];   // C
            // TODO: a.flags = ...
            // TODO: a.regnr = ...

            a.likes = 0; // TODO:

            a.availability = columnVector[3];
            
            // TODO: columnVector[4]; // E unused ?

            a.pack_size = std::stoi(columnVector[5]);
            // TODO: if 0 parse the size from the title
            
            a.therapy_code = columnVector[6];
            if (a.therapy_code.size() == 0)
                a.therapy_code = NO_DETAILS;

            a.atc_code = boost::to_upper_copy<std::string>(columnVector[7]); // H
            if (a.atc_code.size() > 0) {
                a.atc_class = ATC::getTextByAtc(a.atc_code); // Java uses epha_atc_codes_csv.csv instead
            }
            else {
                a.atc_code = NO_DETAILS;
                a.atc_class = NO_DETAILS;
            }

            a.stock = std::stoi(columnVector[8]); // I
            
            a.rose_supplier = columnVector[9]; // J
            
            std::string gal = columnVector[10]; // K
            a.galen_form = gal;
            a.galen_code = gal; // TODO: m_galenic_code_to_form_bimap
            
            a.pack_unit = columnVector[11]; // L
            a.rose_base_price = columnVector[12]; // M TODO: see bag formatPriceAsMoney
            
            std::string ean = columnVector[13];  // N TODO: check that length is 13
            a.replace_ean_code = ean;

            a.replace_pharma_code = columnVector[14]; // O
            
            a.off_the_market = (boost::to_lower_copy<std::string>(columnVector[15]) == "ja"); // P
            a.npl_article = (boost::to_lower_copy<std::string>(columnVector[16]) == "ja"); // Q

            // TODO: use ean and m_bag_public_price_map
            a.public_price = columnVector[17]; // R
            //a.exfactory_price = ...
            
            // Column S unused ?
            
            a.dlk_flag = boost::contains(columnVector[19], "100%"); // T
            a.pack_title_FR = columnVector[20]; // U

            if (type == "fulldb") {
                articles.push_back(a);
            }
            else if ((type == "atcdb") &&
                     (a.atc_code.size() > 0) &&
                     a.atc_code != NO_DETAILS) {
                // Add only products which have an ATC code
                articles.push_back(a);
            }
        }  // while
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
#ifdef DEBUG
    std::clog << "Parsed " << articles.size() << " articles" << std::endl;
#endif
}

} // namespace CSV

#pragma mark -

#ifdef OBSOLETE_STUFF
// See file DispoParse.java line 861
void getAtcMap()
{
    // TODO: parse './downloads/epha_atc_codes_csv.csv'
}
#endif

// See file DispoParse.java line 725
void getSLMap()
{
    // TODO: parse './downloads/bag_preparations_xml.xml'
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
// fulldb,atcdb
void generateFullSQLiteDB(const std::string inDir, const std::string type)
{
    // TODO: use type
    CSV::parseVollstamm(inDir + "/zurrose/artikel_vollstamm_zurrose.csv", type);
    
#ifdef WITH_PROGRESS_BAR
        unsigned int ii = 1;
        int n = articles.size();
#endif
        for (auto a : articles) {
            
#ifdef WITH_PROGRESS_BAR
            // Show progress
            if ((ii++ % 100) == 0)
                std::cerr << "\r" << 100*ii/n << " % ";
#endif
            sqlDb.bindText(1, a.pack_title);
            sqlDb.bindInt(2, a.pack_size);
            sqlDb.bindText(3, a.galen_form);
            sqlDb.bindText(4, a.pack_unit);
            sqlDb.bindText(5, a.ean_code + ";" + a.regnr);
            sqlDb.bindText(6, a.pharma_code);
            sqlDb.bindText(7, a.atc_code + ";" + a.atc_class);
            sqlDb.bindText(8, a.therapy_code);
            sqlDb.bindInt(9, a.stock);
            sqlDb.bindText(10, a.rose_base_price);
            sqlDb.bindText(11, a.availability);
            sqlDb.bindText(12, a.rose_supplier);
            sqlDb.bindInt(13, a.likes);
            sqlDb.bindText(14, a.replace_ean_code);
            sqlDb.bindText(15, a.replace_pharma_code);
            sqlDb.bindBool(16, a.off_the_market);
            sqlDb.bindText(17, a.flags);
            sqlDb.bindBool(18, a.npl_article);
            sqlDb.bindText(19, a.public_price);
            sqlDb.bindText(20, a.exfactory_price);
            sqlDb.bindBool(21, a.dlk_flag);
            sqlDb.bindText(22, a.pack_title_FR);
            sqlDb.bindText(23, a.galen_code);

            sqlDb.runStatement(TABLE_NAME_ROSE);
        } // for
        
#ifdef WITH_PROGRESS_BAR
        std::cerr << "\r100 %" << std::endl;
#endif
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

    KUNDEN::createConditionsJSON(outDir + "/rose_conditions.json"); // .ser in Java
    KUNDEN::createIdsJSON(outDir + "/rose_ids.json"); // .ser in Java
}

// See ShoppingCartRose.java 260 encryptAutoGenerikaToFile()
void generateAutogenerika(std::string inDir, std::string outDir)
{
    GENERIKA::parseCSV(inDir + "/zurrose/Autogenerika.csv");
    GENERIKA::createJSON(outDir + "/rose_autogenerika.json"); // .ser in Java
}

// See ShoppingCartRose.java 303 encryptDirectSubstToFile()
void generateDirect(std::string inDir, std::string outDir)
{
    DIREKT::parseCSV(inDir + "/zurrose/direct_subst_zurrose.csv");
    DIREKT::createJSON(outDir + "/output/rose_direct_subst.json"); // .ser in Java
}

// See ShoppingCartRose.java 339 encryptNotaToFile()
void generateNota(std::string inDir, std::string outDir)
{
    NOTA::parseCSV(inDir + "/zurrose/nota_zurrose.csv");
    NOTA::createJSON(outDir + "/output/rose_nota.json"); // .ser in Java
}

void createDB(const std::string &filename)
{
#ifdef DEBUG
    std::clog << "Create: " << filename << std::endl;
#endif

    sqlDb.openDB(filename);

    // See file DispoParse.java line 187 createArticleDB()
    sqlDb.createTable(TABLE_NAME_ROSE, "_id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, size TEXT, galen TEXT, unit TEXT, eancode TEXT, pharmacode TEXT, atc TEXT, theracode TEXT, stock INTEGER, price TEXT, availability TEXT, supplier TEXT, likes INTEGER, replaceean TEXT, replacepharma TEXT, offmarket TEXT, flags TEXT, npl TEXT, publicprice TEXT, exfprice TEXT, dlkflag TEXT, title_FR TEXT, galencode TEXT");

    sqlDb.createTable(TABLE_NAME_ANDROID, "locale TEXT default 'en_US'");
    sqlDb.insertInto(TABLE_NAME_ANDROID, "locale", "'en_US'");

    //createTable("sqlite_sequence", "");  // created automatically
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
    
    // Parse input files
    // For French names of medicines
    ATC::parseTXT(opt_inputDirectory + "/atc_codes_multi_lingual.txt", opt_language, flagVerbose);
    
    // See ShoppingCartRose.java line 24 encryptFiles()
#ifdef OBSOLETE_STUFF
    // TODO: parse 'Abverkaufszahlen.csv' to generate 'rose_sales_fig.ser'
#endif

    generateKunden(opt_inputDirectory, opt_workDirectory + "/output");
    generateAutogenerika(opt_inputDirectory, opt_workDirectory + "/output");
    generateDirect(opt_inputDirectory, opt_workDirectory);
    generateNota(opt_inputDirectory, opt_workDirectory);

    if ((opt_zurrose == "fulldb") || (opt_zurrose == "atcdb"))
    {
        // See .java initSqliteDB()
        std::string dbName = (opt_zurrose == "fulldb") ? "rose_db_new_full.db" : "rose_db_new_atc_only.db";
        
        std::string dbFullname = opt_workDirectory + "/output/" + dbName;

        createDB(dbFullname);
        
        sqlDb.prepareStatement(TABLE_NAME_ROSE,
                               "null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?");
        
#ifdef OBSOLETE_STUFF
        getAtcMap();
#endif

        getSLMap();

        // Enhance SL map with information on"Abgabekategorie"
        enhanceFlags();

        // Get galenic form to galenic code map
        getGalenicCodeMap();

#ifdef OBSOLETE_STUFF
        processLikes();
#endif

        // Process CSV file and generate Sqlite DB
        generateFullSQLiteDB(opt_inputDirectory, opt_zurrose);
    }
    else if (opt_zurrose == "quick") {
        generatePharmaToStockCsv(opt_inputDirectory, opt_workDirectory + "/output");
    }
    
    if ((opt_zurrose == "fulldb") ||
        (opt_zurrose == "atcdb"))
    {
        sqlDb.destroyStatement();
        sqlDb.closeDB();
    }

    return EXIT_SUCCESS;
}
