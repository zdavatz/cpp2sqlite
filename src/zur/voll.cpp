//
//  voll.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 3 Nov 2019
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>

#include <boost/algorithm/string.hpp>

#include <libgen.h>     // for basename()

#include "voll.hpp"
#include "Article.hpp"
#include "atc.hpp"
#include "sqlDatabase.hpp"
#include "report.hpp"
#include "galen.hpp"
#include "bag.hpp"
#include "bagFHIR.hpp"
#include "swissmedic.hpp"

#define WITH_PROGRESS_BAR

namespace VOLL
{
constexpr std::string_view CSV_SEPARATOR = ";";
constexpr std::string_view NO_DETAILS = "k.A.";    // keine Angaben
constexpr std::string_view TABLE_NAME_ROSE = "rosedb";
constexpr std::string_view TABLE_NAME_ANDROID = "android_metadata";

std::vector<Article> articles;
static DB::Sql sqlDb;

// Parse-phase stats
std::vector<std::string> statsLinesWrongNumFields; // strings so that they can be conveniently joined
unsigned int statsPharmaCodeTooBig = 0;
unsigned int statsPharmaCodeNotNumeric = 0;
unsigned int statsEmptyGalenForm = 0;
unsigned int statsEmptyAtcCode = 0;
unsigned long int statsCsvLineCount = 0;

constexpr long maxAcceptablePharmaCode = 7900000L;
unsigned long int statsFirstLinePharmaOverMax = -1;
unsigned long int statsLineWithPharmaMax = -1;
std::map<std::string, size_t> pharmaArticleIndexMap;

static
void printFileStats(const std::string &filename)
{
    REP::html_h2("ARTIKEL");
    //REP::html_p(std::string(basename((char *)filename.c_str())));
    REP::html_p(filename);

    REP::html_start_ul();
    REP::html_li("Total Articles: " + std::to_string(statsCsvLineCount));
    REP::html_li("Used Articles: " + std::to_string(articles.size()));
    REP::html_li("Column A. 'Pharmacode' > 7900000 (skipped): " + std::to_string(statsPharmaCodeTooBig));
    REP::html_li("Column A. 'Pharmacode' not numeric (skipped): " + std::to_string(statsPharmaCodeNotNumeric));
    REP::html_li("Column H. 'ATC-Key' empty: " + std::to_string(statsEmptyAtcCode));
    REP::html_li("Column K. 'Galen. Form' empty: " + std::to_string(statsEmptyGalenForm));

    if (statsFirstLinePharmaOverMax >= 0)
        REP::html_li("First line with 'Pharmacode' >= 7900000: " + std::to_string(statsFirstLinePharmaOverMax));

    if (statsLineWithPharmaMax >= 0)
        REP::html_li("Line with 'Pharmacode' = 7900000: " + std::to_string(statsLineWithPharmaMax));


    REP::html_end_ul();

    if (statsLinesWrongNumFields.size() > 0) {
        REP::html_p("Lines with wrong number of fields due to embedded separators (Skipped. Valid syntax. Probably 'Pharmacode' >= 7900000). Total count: " + std::to_string(statsLinesWrongNumFields.size()));

        if (statsLinesWrongNumFields.size() > 0)
            REP::html_div(boost::algorithm::join(statsLinesWrongNumFields, ", "));
    }
}

static
std::string parseUnitFromTitle(const std::string &pack_title)
{
    std::regex rgx(R"((\d+)(\.\d+)?\s*(ml|mg|g))");  // tested at https://regex101.com
    std::smatch match;
    std::string dosage;
    if (std::regex_search(pack_title, match, rgx)) {
        dosage = match[0];
    }

    std::string dosage2;
    std::regex rgx2(R"(((\d+)(\.\d+)?(Ds|ds|mg)?)(\/(\d+)(\.\d+)?\s*(Ds|ds|mg|ml|mg|g)?)+)");  // tested at https://regex101.com
    if (std::regex_search(pack_title, match, rgx2)) {
        dosage2 = match[0];
    }
    if (dosage.empty() || boost::algorithm::contains(dosage2, dosage)) {
        return dosage2;
    }

    if (dosage.length() > 0) {
        return dosage;
    }

    return {};
}

void parseCSV(const std::string &filename,
              const std::string type,
              bool flagFHIR,
              bool dumpHeader)
{
    std::cout << "Reading " << filename << std::endl;

    try {
        std::ifstream file(filename);

        std::string str;
        bool header = true;
        while (std::getline(file, str))
        {
            boost::algorithm::trim_right_if(str, boost::is_any_of(" \n\r"));
            statsCsvLineCount++;

            if (header) {
                header = false;

                if (dumpHeader) {
                    std::ofstream outHeader(filename + ".header.txt");
                    std::vector<std::string> headerTitles;
                    boost::algorithm::split(headerTitles, str, boost::is_any_of(CSV_SEPARATOR));
                    outHeader << "Number of columns: " << headerTitles.size() << std::endl;
                    auto colLetter = 'A';
                    int index = 0;
                    for (auto t : headerTitles)
                        outHeader << index++ << ") " << colLetter++ << "\t" << t << std::endl;

                    outHeader.close();
                }

                continue;
            }

            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(CSV_SEPARATOR));

            if (columnVector.size() != 21) {
#ifdef DEBUG
                std::clog
                << "CSV line: " << statsCsvLineCount
                << ", unexpected # columns: " << columnVector.size() << std::endl;
#endif
                statsLinesWrongNumFields.push_back(std::to_string(statsCsvLineCount));
                // TODO: we could retry splitting the line with a proper CSV parser
                // but these lines have pharmacode >= 7900000 anyway and they would be skipped later
                continue;
            }

            // Validate column A
            auto pharmaCode = columnVector[0]; // A
            try {
                auto numericCode = std::stol(pharmaCode);

                if (numericCode == maxAcceptablePharmaCode)
                    statsLineWithPharmaMax = statsCsvLineCount;

                if (numericCode >= maxAcceptablePharmaCode) {
                    // The file seems to be sorted by column A.
                    // therefore all following lines will fail this validation.
                    // We could abort parsing here, but we continue to collects stats on the file.
                    statsPharmaCodeTooBig++;

                    //std::cerr << " CSV " << statsCsvLineCount << ", " << str << std::endl;
                    if (statsFirstLinePharmaOverMax == -1)
                        statsFirstLinePharmaOverMax = statsCsvLineCount;

                    continue;
                }
            }
            catch (std::exception &e) {
                statsPharmaCodeNotNumeric++;
                //std::cerr << "Line: " << __LINE__ << " Error " << e.what() << std::endl;
                continue;
            }

            // Validate column H
            auto atcCode = columnVector[7]; // H
            if (atcCode.length() == 0)
            {
                statsEmptyAtcCode++;
                if (type == "atcdb")
                    continue;
            }

            Article a;
            a.pharma_code = pharmaCode;
            a.pack_title = columnVector[1];

            // See file DispoParse.java line 417
            a.ean_code = columnVector[2];   // C
            if ((a.ean_code.length() == 13) &&
                (a.ean_code.substr(0, 4) == "7680"))
            {
                auto cat = SWISSMEDIC::getCategoryPackByGtin(a.ean_code);
                std::string paf;
                if (flagFHIR) {
                    paf = BAGFHIR::getPricesAndFlags(a.ean_code, cat); // update BAG::packMap within this GTIN
                } else {
                    paf = BAG::getPricesAndFlags(a.ean_code, cat); // update BAGFHIR::packMap within this GTIN
                }
                if (paf.length() > 0) {
                    BAG::packageFields fromBag;
                    if (flagFHIR) {
                        fromBag = BAGFHIR::getPackageFieldsByGtin(a.ean_code); // get it from BAG::packMap
                    } else {
                        fromBag = BAG::getPackageFieldsByGtin(a.ean_code); // get it from BAGFHIR::packMap
                    }

                    auto bioT = SWISSMEDIC::getCategoryMedByGtin(a.ean_code);
                    if (bioT == "Biotechnologika") {
                        //a.flags += ", BioT";
                        fromBag.flags.push_back("BioT");
                    }

                    a.flags = boost::algorithm::join(fromBag.flags, ", ") ;
                    a.exfactory_price = fromBag.efp;
                }
                a.regnr = a.ean_code.substr(4, 5); // pos, len
            }

            a.likes = 0; // TODO:

            a.availability = columnVector[3];

            // TODO: columnVector[4]; // E unused ?

            a.pack_size = std::stoi(columnVector[5]);
            // TODO: if 0 parse the size from the title

            a.therapy_code = columnVector[6];
            if (a.therapy_code.size() == 0)
                a.therapy_code = NO_DETAILS;

            if (atcCode.size() == 0) {
                a.atc_code = NO_DETAILS;
                a.atc_class = NO_DETAILS;
            }
            else {
                a.atc_code = boost::to_upper_copy<std::string>(atcCode);
                a.atc_class = ATC::getTextByAtc(a.atc_code); // Java uses epha_atc_codes_csv.csv instead
            }

            //if (columnVector[8].length() > 0)
            a.stock = std::stoi(columnVector[8]); // I

            a.rose_supplier = columnVector[9]; // J

            auto gal = columnVector[10]; // K
            if (gal.length() == 0) {
                statsEmptyGalenForm++;
            }
            else {
                try {
                    auto numericCode = std::stoi(gal);
                    a.galen_form = GALEN::getTextByCode(numericCode);
                    a.galen_code = gal;
                }
                catch (std::exception &e) {
                    a.galen_form = gal;
                    //a.galen_code = TODO: reverse lookup ?
                }
            }

            // See file DispoParse.java line 497
            a.pack_unit = columnVector[11]; // L
            if ((a.pack_unit.length() == 0) || (a.pack_unit == "0")) {
                a.pack_unit = parseUnitFromTitle(a.pack_title); // get it from column B
            }

            a.rose_base_price = columnVector[12]; // M TODO: see bag formatPriceAsMoney

            auto ean = columnVector[13];  // N TODO: check that length is 13
            a.replace_ean_code = ean;

            a.replace_pharma_code = columnVector[14]; // O

            a.off_the_market = (boost::to_lower_copy<std::string>(columnVector[15]) == "ja"); // P
            a.npl_article = (boost::to_lower_copy<std::string>(columnVector[16]) == "ja"); // Q

            // TODO: use ean and m_bag_public_price_map
            a.public_price = columnVector[17]; // R

            a.dispose_flag = columnVector[18]; // S

            a.dlk_flag = boost::contains(columnVector[19], "100%"); // T
            a.pack_title_FR = columnVector[20]; // U

            articles.push_back(a);
            pharmaArticleIndexMap.insert(std::make_pair(a.pharma_code, articles.size()-1));
        }  // while

        file.close();
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", CSV line: " << statsCsvLineCount
        << ",  Error " << e.what()
        << std::endl;
    }

    printFileStats(filename);
}

void parseNonFullCSV(const std::string &filename,
                     const std::string type,
                     bool dumpHeader)
{
    std::cout << "Reading " << filename << std::endl;

    try {
        std::ifstream file(filename);

        int patchedCount = 0;
        std::string str;
        bool header = true;
        while (std::getline(file, str))
        {
            if (header) {
                header = false;

                if (dumpHeader) {
                    std::ofstream outHeader(filename + ".header.txt");
                    std::vector<std::string> headerTitles;
                    boost::algorithm::split(headerTitles, str, boost::is_any_of(CSV_SEPARATOR));
                    outHeader << "Number of columns: " << headerTitles.size() << std::endl;
                    auto colLetter = 'A';
                    for (auto t : headerTitles)
                        outHeader << colLetter++ << "\t" << t << std::endl;

                    outHeader.close();
                }

                continue;
            }

            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(CSV_SEPARATOR));

            if (columnVector.size() != 21) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }

            std::string pharma = columnVector[0]; // A
            if (pharma.length() > 0) {
                int stock = std::stoi(columnVector[8]); // I

                auto search = pharmaArticleIndexMap.find(pharma);
                if (search != pharmaArticleIndexMap.end()) {
                    size_t index = search->second;
                    auto a = articles.at(index);
                    if (a.stock != stock) {
                        a.stock = stock;
                        patchedCount++;
                    }
                    articles.at(index) = a;
                }
            }
        } // while

        file.close();
        std::clog << "Patched stock count for " << std::to_string(patchedCount) << " articles" << std::endl;
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

// See DispoParse.java:65
void openDB(const std::string &filename)
{
    std::clog << std::endl << "Create DB: " << filename << std::endl;

    sqlDb.openDB(filename);

    // See file DispoParse.java line 187 createArticleDB()
    sqlDb.createTable(TABLE_NAME_ROSE, "_id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, size TEXT, galen TEXT, unit TEXT, eancode TEXT, pharmacode TEXT, atc TEXT, theracode TEXT, stock INTEGER, price TEXT, availability TEXT, supplier TEXT, likes INTEGER, replaceean TEXT, replacepharma TEXT, offmarket TEXT, flags TEXT, npl TEXT, publicprice TEXT, exfprice TEXT, dlkflag TEXT, title_FR TEXT, galencode TEXT, disposeflag TEXT");

    sqlDb.createTable(TABLE_NAME_ANDROID, "locale TEXT default 'en_US'");
    sqlDb.insertInto(TABLE_NAME_ANDROID, "locale", "'en_US'");

    //createTable("sqlite_sequence", "");  // created automatically

    sqlDb.prepareStatement(TABLE_NAME_ROSE,
                           "null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?");
}

void createDB()
{
    std::clog << "Writing DB" << std::endl;

#ifdef WITH_PROGRESS_BAR
    unsigned int ii = 1;
    int n = articles.size();
#endif
    for (auto a : articles) {

#ifdef WITH_PROGRESS_BAR
        // Show progress
        if ((ii++ % 300) == 0)
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
        sqlDb.bindText(24, a.dispose_flag);

        sqlDb.runStatement(TABLE_NAME_ROSE);
    } // for

#ifdef WITH_PROGRESS_BAR
        std::cerr << "\r100 %" << std::endl;
#endif
}

void closeDB()
{
    sqlDb.destroyStatement();
    sqlDb.closeDB();
}

}
