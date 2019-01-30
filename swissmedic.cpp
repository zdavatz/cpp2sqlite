//
//  swissmedic.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 22 Jan 2019
//

#include <iostream>
#include <libgen.h>     // for basename()
#include <regex>
#include <boost/algorithm/string.hpp>

#include <xlnt/xlnt.hpp>

#include "swissmedic.hpp"
#include "gtin.hpp"
#include "bag.hpp"
#include "beautify.hpp"

#define COLUMN_A        0   // GTIN (5 digits)
#define COLUMN_C        2   // name
#define COLUMN_G        6   // ATC
#define COLUMN_K       10   // packaging code (3 digits)
#define COLUMN_L       11   // number for dosage
#define COLUMN_M       12   // units for dosage
#define COLUMN_N       13   // category (A..E)
#define COLUMN_S       18   // application field
#define COLUMN_W       22   // preparation contains narcotics

#define FIRST_DATA_ROW_INDEX    5

namespace SWISSMEDIC
{
    std::vector< std::vector<std::string> > theWholeSpreadSheet;
    std::vector<std::string> regnrs;        // padded to 5 characters (digits)
    std::vector<std::string> packingCode;   // padded to 3 characters (digits)
    std::vector<std::string> gtin;
    std::vector<std::string> category;
    
    int statsAugmentedRegnCount = 0;
    int statsAugmentedGtinCount = 0;
    int statsTotalGtinCount = 0;
    int statsRecoveredDosage = 0;

void parseXLXS(const std::string &filename)
{
    xlnt::workbook wb;
    wb.load(filename);
    auto ws = wb.active_sheet();

    std::clog << std::endl << "Reading swissmedic XLSX" << std::endl;

    int skipHeaderCount = 0;
    for (auto row : ws.rows(false)) {
        if (++skipHeaderCount <= FIRST_DATA_ROW_INDEX)
            continue;

        std::vector<std::string> aSingleRow;
        for (auto cell : row) {
            //std::clog << cell.to_string() << std::endl;
            aSingleRow.push_back(cell.to_string());
        }

        theWholeSpreadSheet.push_back(aSingleRow);

        // Precalculate padded regnr
        std::string rn5 = GTIN::padToLength(5, aSingleRow[COLUMN_A]);
        regnrs.push_back(rn5);

        // Precalculate padded packing code
        std::string code3 = GTIN::padToLength(3, aSingleRow[COLUMN_K]);
        packingCode.push_back(code3);
        
        // Precalculate gtin
        std::string gtin12 = "7680" + rn5 + code3;
        char checksum = GTIN::getGtin13Checksum(gtin12);
        gtin.push_back(gtin12 + checksum);

        // Precalculate category
        std::string cat = aSingleRow[COLUMN_N];
        if ((cat == "A") && (aSingleRow[COLUMN_W] == "a"))
            cat += "+";
        
        category.push_back(cat);
    }

    std::clog << "swissmedic rows: " << theWholeSpreadSheet.size() << std::endl;
}
    
std::string getAdditionalNames(const std::string &rn,
                               std::set<std::string> &gtinUsed)
{
    std::string names;
    int i=0;
    std::set<std::string>::iterator it;
    bool statsGtinAdded = false;
    
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        std::string rn5 = regnrs[rowInt];
        if (rn5 != rn)
            continue;
        
        std::string g13 = gtin[rowInt];
        it = gtinUsed.find(g13);
        if (it == gtinUsed.end()) { // not found list of used GTINs, we must add the name
            statsGtinAdded = true;
            statsAugmentedGtinCount++;
            statsTotalGtinCount++;
            gtinUsed.insert(g13);   // also update the list of GTINs used so far
            if (i++ > 0)
                names += "\n";

            std::string name = theWholeSpreadSheet.at(rowInt).at(COLUMN_C);
            name = BEAUTY::beautifyName(name);
            // Verify presence of dosage
            std::regex r(R"(\d+)");
            if (!std::regex_search(name, r)) {
                statsRecoveredDosage++;
                //std::clog << "no dosage for " << name << std::endl;
                std::string dosage = theWholeSpreadSheet.at(rowInt).at(COLUMN_L);
                std::string units = theWholeSpreadSheet.at(rowInt).at(COLUMN_M);
                name += " " + dosage + " " + units;
            }

#ifdef DEBUG_IDENTIFY_NAMES
            names += "swm+";
#endif
            names += name;
            
            std::string paf = BAG::getPricesAndFlags(g13,
                                                     "ev.nn.i.H.", // TODO: localize
                                                     category[rowInt]);
            if (!paf.empty())
                names += paf;
        }
    }
    
    if (statsGtinAdded)
        statsAugmentedRegnCount++;

    return names;    
}

int countRowsWithRn(const std::string &rn)
{
    int count = 0;
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        std::string gtin_5 = regnrs[rowInt];
        // TODO: to speed up do a numerical comparison so that we can return when gtin5>rn
        // assuming that column A is sorted
        if (gtin_5 == rn)
            count++;
    }

    return count;
}
    
bool findGtin(const std::string &gtin)
{
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        std::string rn5 = regnrs[rowInt];
        std::string code3 = packingCode[rowInt];
        std::string gtin12 = "7680" + rn5 + code3;

#if 0
        // We could also recalculate and verify the checksum
        // but such verification has already been done when parsing the files
        char checksum = GTIN::getGtin13Checksum(gtin12);
        
        if (checksum != gtin[12]) {
            std::cerr
            << basename((char *)__FILE__) << ":" << __LINE__
            << ", GTIN error, expected:" << checksum
            << ", received" << gtin[12]
            << std::endl;
        }
#endif

        // The comparison is only the first 12 digits, without checksum
        if (gtin12 == gtin.substr(0,12)) // pos, len
            return true;
    }

    return false;
}

std::string getApplication(const std::string &rn)
{
    std::string app;
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        if (rn == regnrs[rowInt]) {
            app = theWholeSpreadSheet.at(rowInt).at(COLUMN_S) + " (Swissmedic)";
            break;
        }
    }

    return app;
}

std::string getAtcFromFirstRn(const std::string &rn)
{
    std::string atc;

    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        if (rn == regnrs[rowInt]) {
            atc = theWholeSpreadSheet.at(rowInt).at(COLUMN_G);
            break;
        }
    }

    return atc;
}

std::string getCategoryFromGtin(const std::string &g)
{
    std::string cat;

    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++)
        if (gtin[rowInt] == g) {
            cat = category[rowInt];
            break;
        }

    return cat;
}

void printStats()
{
    std::cout
    << "GTINs used from swissmedic " << statsTotalGtinCount
    << ", recovered dosage " << statsRecoveredDosage
    << std::endl;
}
    
}
