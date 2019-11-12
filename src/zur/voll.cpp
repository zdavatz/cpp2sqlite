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

#include <boost/algorithm/string.hpp>

#include <libgen.h>     // for basename()

#include "voll.hpp"
#include "Article.hpp"
#include "atc.hpp"
#include "sqlDatabase.hpp"
#include "report.hpp"

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
unsigned int statsPharmaCodeTooBig = 0;
unsigned int statsPharmaCodeNotNumeric = 0;

static
void printFileStats(const std::string &filename)
{
    REP::html_h2("ARTIKEL");
    //REP::html_p(std::string(basename((char *)filename.c_str())));
    REP::html_p(filename);

    REP::html_start_ul();
    REP::html_li("Articles: " + std::to_string(articles.size()));
    REP::html_li("pharma code > 7900000: " + std::to_string(statsPharmaCodeTooBig));
    REP::html_li("pharma code not numeric: " + std::to_string(statsPharmaCodeNotNumeric));
    REP::html_end_ul();
}

void parseCSV(const std::string &filename,
              const std::string type,
              bool dumpHeader)
{
    std::clog << std::endl << "Reading " << filename << std::endl;
    
    try {
        std::ifstream file(filename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str)) {
            
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
            
            try {
                auto numericCode = std::stol(columnVector[0]);
                if (numericCode > 7900000L) {
                    statsPharmaCodeTooBig++;
                    continue;
                }
            }
            catch (std::exception &e) {
                statsPharmaCodeNotNumeric++;
                //std::cerr << "Line: " << __LINE__ << " Error " << e.what() << std::endl;
                continue;
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
    
    printFileStats(filename);
}

void openDB(const std::string &filename)
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
    
    sqlDb.prepareStatement(TABLE_NAME_ROSE,
                           "null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?");
}

void createDB()
{
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

void closeDB()
{
    sqlDb.destroyStatement();
    sqlDb.closeDB();
}

}
