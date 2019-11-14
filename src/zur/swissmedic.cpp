//
//  swissmedic.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 14 Nov 2019
//

#include <iostream>
#include <fstream>
#include <libgen.h>     // for basename()
#include <regex>
#include <iomanip>
#include <sstream>
#include <boost/algorithm/string.hpp>

#include <xlnt/xlnt.hpp>

#include "swissmedic.hpp"
//#include "swissmedic2.hpp"
#include "gtin.hpp"
#include "bag.hpp"
//#include "beautify.hpp"
#include "report.hpp"
//#include "refdata.hpp"
//#include "ddd.hpp"
//#include "aips.hpp"

#define WITH_PROGRESS_BAR

#define COLUMN_A        0   // GTIN (5 digits)
#define COLUMN_B        1   // dosage number
#define COLUMN_C        2   // name
#define COLUMN_D        3   // owner
#define COLUMN_E        4   // category
#define COLUMN_F        5   // IT number
#define COLUMN_G        6   // ATC
#define COLUMN_H        7   // registration date (Date d'autorisation du dosage)
#define COLUMN_J        9   // valid until (Durée de validité de l'AMM))
#define COLUMN_K       10   // packaging code (3 digits)
#define COLUMN_L       11   // number for dosage
#define COLUMN_M       12   // units for dosage
#define COLUMN_N       13   // category (A..E)
#define COLUMN_T       19   // application field
#define COLUMN_W       22   // preparation contains narcotics
#define COLUMN_X       23   // narcotic flag

#define FIRST_DATA_ROW_INDEX    6

static constexpr std::string_view CELL_ESCAPE = "\"";

namespace SWISSMEDIC
{
    std::vector< std::vector<std::string> > theWholeSpreadSheet;
//    std::vector<std::string> regnrs;        // padded to 5 characters (digits)
#if 1
    std::vector<pharmaRow> pharmaVec;
#else
    std::vector<std::string> packingCode;   // padded to 3 characters (digits)
    std::vector<std::string> gtin;
#endif

//    std::string fromSwissmedic("ev.nn.i.H.");
    
//    // TODO: change it to a map for better performance
//    std::vector<std::string> categoryVec;
//
//    // TODO: change them to a map for better performance
//    std::vector<dosageUnits> duVec;

    // Parse-phase stats


static
void printFileStats(const std::string &filename)
{
    REP::html_h2("Swissmedic");
    //REP::html_p(std::string(basename((char *)filename.c_str())));
    REP::html_p(filename);
    REP::html_start_ul();
    REP::html_li("rows: " + std::to_string(theWholeSpreadSheet.size()));
    REP::html_end_ul();
}

void parseXLXS(const std::string &downloadDir, bool dumpHeader)
{
    const std::string &filename = downloadDir + "/swissmedic_packages.xlsx";

    std::clog << std::endl << "Reading " << filename << std::endl;

    xlnt::workbook wb;
    wb.load(filename);
    auto ws = wb.active_sheet();
    
    auto date_format = wb.create_format().number_format(xlnt::number_format{"dd.mm.yyyy"}, xlnt::optional<bool>(true));

    unsigned int skipHeaderCount = 0;
    for (auto row : ws.rows(false)) {
        ++skipHeaderCount;
        if (skipHeaderCount <= FIRST_DATA_ROW_INDEX) {
            
            if (dumpHeader &&
                (skipHeaderCount == FIRST_DATA_ROW_INDEX))
            {
                std::ofstream outHeader(filename + ".header.txt");
                int i=0;
                for (auto cell : row) {
                    xlnt::column_t::index_t col_idx = cell.column_index();
                    outHeader << i++ << "\t" << xlnt::column_t::column_string_from_index(col_idx) << std::endl
                    << "<" << cell.to_string() << ">" << std::endl
                    << std::endl;
                }
                outHeader.close();
            }

            continue;
        }

        std::vector<std::string> aSingleRow;
        for (auto cell : row) {
            if (cell.is_date()) {
                cell.format(date_format);
                auto nf = cell.number_format();
                aSingleRow.push_back(nf.format(std::stoi(cell.to_string()), xlnt::calendar::windows_1900));
            }
            else {
                aSingleRow.push_back(cell.to_string());
            }
        }

        theWholeSpreadSheet.push_back(aSingleRow);

        pharmaRow pr;
        pr.rn5 = GTIN::padToLength(5, aSingleRow[COLUMN_A]);
#if 0
        regnrs.push_back(pr.rn5);

        pr.dosageNr = aSingleRow[COLUMN_B];
#endif
        pr.code3 = GTIN::padToLength(3, aSingleRow[COLUMN_K]);
        
        // Precalculate gtin
        std::string gtin12 = "7680" + pr.rn5 + pr.code3;
        char checksum = GTIN::getGtin13Checksum(gtin12);
        pr.gtin13 = gtin12 + checksum;

        std::string nameString = aSingleRow[COLUMN_C];
        boost::replace_all(nameString, CELL_ESCAPE, ""); // double quotes interfere with CSV
        boost::replace_all(nameString, ";", ","); // sometimes the galenic form is after ';' not ','

        std::vector<std::string> nameComponents;
        boost::algorithm::split(nameComponents, nameString, boost::is_any_of(","));

#if 0
        // Use the name only up to first comma (Issue #68, 5)
        pr.name = nameComponents[0];

        // Use words after last comma (Issue #68, 6)
        pr.galenicForm = nameComponents[nameComponents.size()-1];
        boost::algorithm::trim(pr.galenicForm);

        pr.owner = aSingleRow[COLUMN_D];

        pr.categoryMed = aSingleRow[COLUMN_E];
        if ((pr.categoryMed != "Impfstoffe") && (pr.categoryMed != "Blutprodukte"))
            pr.categoryMed.clear();
        
        pr.itNumber = aSingleRow[COLUMN_F];
        pr.regDate = aSingleRow[COLUMN_H];      // Date
        pr.validUntil = aSingleRow[COLUMN_J];   // Date
        pr.du.dosage = aSingleRow[COLUMN_L];
        pr.du.units = aSingleRow[COLUMN_M];
        pr.narcoticFlag = aSingleRow[COLUMN_X];
#endif

        pr.categoryPack = aSingleRow[COLUMN_N];
        if ((pr.categoryPack == "A") && (aSingleRow[COLUMN_W] == "a"))
            pr.categoryPack += "+";

        pharmaVec.push_back(pr);
    }

    printFileStats(filename);
}

std::string getCategoryPackByGtin(const std::string &g)
{
    std::string cat;

    for (auto pv : pharmaVec)
        if (pv.gtin13 == g) {
            cat = pv.categoryPack;
            break;
        }

    return cat;
}

}
