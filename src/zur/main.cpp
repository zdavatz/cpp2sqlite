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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <libgen.h>     // for basename()

#include "config.h"
#include "sqlDatabase.hpp"
#include "Article.hpp"
#include "atc.hpp"

#define WITH_PROGRESS_BAR
#define TABLE_NAME_ROSE     "rosedb"
#define NO_DETAILS          "k.A."  // keine Angaben

namespace po = boost::program_options;
namespace pt = boost::property_tree;

static std::string appName;

static DB::Sql sqlDb;
std::vector<Article> articles;

struct stockStruct {
    int zurrose {0};
    int voigt {0};
};
std::map<std::string, stockStruct> pharmaStockMap;

#pragma mark - CSV

namespace CSV
{

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
                boost::algorithm::split(headerTitles, str, boost::is_any_of(";"));
                std::clog << "Number of columns: " << headerTitles.size() << std::endl;
                auto colLetter = 'A';
                for (auto t : headerTitles)
                    std::clog << colLetter++ << "\t" << t << std::endl;
#endif
                continue;
            }
            
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(";"));
            
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
            if (a.therapy_code.size() > 0)
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

void parseStamm(const std::string &filename)
{
    std::clog << std::endl << "Reading artikel_stamm_zurrose CSV" << std::endl;

    try {
        std::ifstream file(filename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str)) {
            
            if (header) {
                header = false;

#ifdef DEBUG
                std::vector<std::string> headerTitles;
                boost::algorithm::split(headerTitles, str, boost::is_any_of(";"));
                std::clog << "Number of columns: " << headerTitles.size() << std::endl;
                auto colLetter = 'A';
                for (auto t : headerTitles)
                    std::clog << colLetter++ << "\t" << t << std::endl;
#endif
                continue;
            }
            
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(";"));
            
            if (columnVector.size() != 21) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }
            
            std::string pharma = columnVector[0]; // A
            if (pharma.length() > 0) {
                stockStruct stock;
                stock.zurrose = std::stoi(columnVector[8]); // I
                pharmaStockMap.insert(std::make_pair(pharma, stock));
            }
        } // while
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
#ifdef DEBUG
    std::clog << "Parsed " << pharmaStockMap.size() << " map items" << std::endl;
#endif
}

// Parse-phase stats
unsigned int statsVoigtNumValidLines = 0;
unsigned int statsVoigtEmptyAnot7 = 0;
unsigned int statsVoigtEmptyB = 0;
unsigned int statsVoigtMapAdditions = 0;
unsigned int statsVoigtMapUpdates = 0;

void parseStammVoigt(const std::string &filename)
{
    std::clog << std::endl << "Reading artikel_stamm_voigt CSV" << std::endl;

    try {
        std::ifstream file(filename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str))
        {
            // Trim the line, otherwise when it's split in cells the B
            // column instead of being an empty cell it contains '\r'
            boost::algorithm::trim_right_if(str, boost::is_any_of("\n\r"));

            if (header) {
                header = false;

#ifdef DEBUG
                std::vector<std::string> headerTitles;
                boost::algorithm::split(headerTitles, str, boost::is_any_of(";"));
                std::clog << "Number of columns: " << headerTitles.size() << std::endl;
                auto colLetter = 'A';
                for (auto t : headerTitles)
                    std::clog << colLetter++ << "\t" << t << std::endl;
#endif
                continue;
            }
            
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(";"));
            
            if (columnVector.size() != 2) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }

            statsVoigtNumValidLines++;
            std::string pharma = columnVector[0];
            int voigtStock = 0;
            if (columnVector[1].length() > 0)
                voigtStock = std::stoi(columnVector[1]);
            else
                statsVoigtEmptyB++;

            if (pharma.length() == 7)
            {
                auto search = pharmaStockMap.find(pharma);
                if (search != pharmaStockMap.end()) {
                    //stock = search->second;   // Get saved stock
                    search->second.voigt = voigtStock; // Update saved stock
                    statsVoigtMapUpdates++;
                }
                else {
                    stockStruct stock {0, voigtStock};
                    pharmaStockMap.insert(std::make_pair(pharma, stock));
                    statsVoigtMapAdditions++;
                }
            }
            else {
                statsVoigtEmptyAnot7++;
            }
        } // while
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
#ifdef DEBUG
    std::clog
    << "Parsed " << statsVoigtNumValidLines << " valid lines"
    << "\n\t" << statsVoigtMapUpdates+statsVoigtMapAdditions << " = "
        << statsVoigtMapUpdates << " map updates + "
        << statsVoigtMapAdditions << " map additions"
    << "\n\t" << statsVoigtEmptyAnot7 << " A column code length not 7"
    << "\n\t" << statsVoigtEmptyB << " B column empty"
    << "\n\t" << pharmaStockMap.size() << " items in stock map"
    << std::endl;
#endif
}

void createRoseStock(const std::string &filename)
{
    std::ofstream ofs;
    ofs.open(filename);
    constexpr std::string_view OUTPUT_FILE_SEPARATOR = ";";
    
    std::clog << std::endl << "Creating CSV" << std::endl;
    
    for (auto item : pharmaStockMap) {
        ofs
        << item.first << OUTPUT_FILE_SEPARATOR          // A
        << item.second.zurrose << OUTPUT_FILE_SEPARATOR // B
        << item.second.voigt << std::endl;              // C
    }
    
    ofs.close();

#ifdef DEBUG
    std::clog << std::endl << "Created " << filename << std::endl;
#endif
}

} // namespace CSV

#pragma mark - CSV_JSON

namespace CSV_JSON
{
struct notaPosition {
    std::string pharma_code;
    std::string quantity;
    std::string status;
    std::string delivery_date;
    std::string last_order_date;
};

struct notaDoctor {
    std::string id;
    std::vector<notaPosition> notaVec;
};

std::vector<notaDoctor> doctorVec;

// Parse-phase stats
unsigned int statsNotaNumLines = 0;

void parseNota(const std::string &filename)
{
    std::clog << "Reading " << filename << std::endl;
    try {
        std::ifstream file(filename);
        
        std::string str;

        while (std::getline(file, str)) {

            boost::algorithm::trim_right_if(str, boost::is_any_of("\n\r"));
            statsNotaNumLines++;

            // No header
            
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(";"));

            // Variable number of columns, but in multiples of 5, plus 1

            auto n = columnVector.size();
            if ((n - 1) % 5 != 0) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }

            notaDoctor doctor;
            doctor.id = columnVector[0];
            doctor.notaVec.clear();

            for (int i=1; i<=(n-5); i+= 5) {
                notaPosition np;
                np.pharma_code = columnVector[i];
                np.quantity = columnVector[i+1];
                np.status = columnVector[i+2];
                np.delivery_date = columnVector[i+3]; // could be empty
                np.last_order_date = columnVector[i+4];
                doctor.notaVec.push_back(np);
            }

            // TODO:: stats update max num of items per doctor from doctor.notaVec.size()

            doctorVec.push_back(doctor);
        }
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
#ifdef DEBUG
    std::clog
    << "Parsed " << statsNotaNumLines << " lines"
    << "\n\t" << doctorVec.size() << " doctors"
    << std::endl;
#endif
}

void createNota(const std::string &filename)
{
    std::clog << "Preparing data for rose_nota.json"  << std::endl;

    pt::ptree tree;

    for (auto d : doctorVec) {

        pt::ptree children;

        for (auto n : d.notaVec) {

            pt::ptree child;

            child.put("delivery_date", n.delivery_date);
            child.put("last_order_date", n.last_order_date);
            child.put("pharma_code", n.pharma_code);
            
            // FIXME: boost seems to treat everything as strings
            // possible solution is https://github.com/nlohmann/json#json-as-first-class-data-type
            child.put("quantity", std::stoi(n.quantity));

            child.put("status", n.status);
            children.push_back(std::make_pair("", child));
        }

        tree.add_child(d.id, children);
    }

    std::clog << "\nWriting (Boost)" << filename << std::endl;
    pt::write_json(filename, tree);
}

} // namespace CSV_JSON

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
void generateFullSQLiteDB(const std::string dir, const std::string type)
{
    // TODO: use type
    CSV::parseVollstamm(dir + "/zurrose/artikel_vollstamm_zurrose.csv", type);
    
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
    CSV::parseStamm(inDir + "/zurrose/artikel_stamm_zurrose.csv");
    CSV::parseStammVoigt(inDir + "/zurrose/artikel_stamm_voigt.csv");
    
    CSV::createRoseStock(outDir + "/rose_stock.csv");
}

void generateNota(std::string inFilename, std::string outFilename)
{
    CSV_JSON::parseNota(inFilename);
    CSV_JSON::createNota(outFilename);
}

void createDB(const std::string &filename)
{
#ifdef DEBUG
    std::clog << "Create: " << filename << std::endl;
#endif

    sqlDb.openDB(filename);

    // See file DispoParse.java line 187 createArticleDB()
    sqlDb.createTable(TABLE_NAME_ROSE, "_id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, size TEXT, galen TEXT, unit TEXT, eancode TEXT, pharmacode TEXT, atc TEXT, theracode TEXT, stock INTEGER, price TEXT, availability TEXT, supplier TEXT, likes INTEGER, replaceean TEXT, replacepharma TEXT, offmarket TEXT, flags TEXT, npl TEXT, publicprice TEXT, exfprice TEXT, dlkflag TEXT, title_FR TEXT, galencode TEXT");
#if 0
    sqlDb.createIndex(TABLE_NAME_ROSE, "idx_", {"title", "auth", "atc", "substances", "regnrs", "atc_class"});

    sqlDb.createTable("productdb", "_id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, author TEXT, eancodes TEXT, pack_info_str TEXT, packages TEXT");
    sqlDb.createIndex("productdb", "idx_prod_", {"title", "author", "eancodes"});
    
    sqlDb.createTable("android_metadata", "locale TEXT default 'en_US'");
    sqlDb.insertInto("android_metadata", "locale", "'en_US'");
#endif

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
    // TODO: parse 'Kunden_alle.csv' to generate 'Kunden_alle.ser' and 'rose_ids.ser'
    // TODO: parse 'Autogenerika.csv' to generate 'rose_autogenerika.ser'
    // TODO: parse 'direct_subst_zurrose.csv' to generate 'rose_direct_subst.ser'
    generateNota(opt_inputDirectory + "/zurrose/nota_zurrose.csv",
                 opt_workDirectory + "/output/rose_nota.json"); // .ser in Java

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
