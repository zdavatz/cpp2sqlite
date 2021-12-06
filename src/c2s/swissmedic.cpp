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
#include "report.hpp"

#define COLUMN_A        0   // GTIN (5 digits)
#define COLUMN_C        2   // name
#define COLUMN_G        6   // ATC
#define COLUMN_K       10   // packaging code (3 digits)
#define COLUMN_L       11   // number for dosage
#define COLUMN_M       12   // units for dosage
#define COLUMN_N       13   // category (A..E)
#define COLUMN_T       19   // application field
#define COLUMN_W       22   // preparation contains narcotics

#define FIRST_DATA_ROW_INDEX    5

namespace SWISSMEDIC
{
    std::vector< std::vector<std::string> > theWholeSpreadSheet;
    std::vector<std::string> regnrs;        // padded to 5 characters (digits)
    std::vector<std::string> packingCode;   // padded to 3 characters (digits)
    std::vector<std::string> gtin;
    std::string fromSwissmedic("ev.nn.i.H.");
    
    // TODO: change it to a map for better performance
    std::vector<std::string> categoryVec;

    // TODO: change them to a map for better performance
    std::vector<dosageUnits> duVec;

    // Parse-phase stats

    // Usage stats
    unsigned int statsAugmentedRegnCount = 0;
    unsigned int statsAugmentedGtinCount = 0;    
    unsigned int statsTotalGtinCount = 0;
    unsigned int statsRecoveredDosage = 0;

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

void printUsageStats()
{
    REP::html_h2("Swissmedic");
    
    REP::html_start_ul();
    REP::html_li("GTINs used: " + std::to_string(statsTotalGtinCount));
    REP::html_li("recovered dosage " + std::to_string(statsRecoveredDosage));
    REP::html_end_ul();
}

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
        int j = 0;
        for (auto cell : row) {
            //std::clog << cell.to_string() << std::endl;
            aSingleRow.push_back(cell.to_string());
            j++;
            if (j >= 25) break;
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
        {
        std::string cat = aSingleRow[COLUMN_N];
        if ((cat == "A") && (aSingleRow[COLUMN_W] == "a"))
            cat += "+";
        
        categoryVec.push_back(cat);
        }
        
        // Precalculate dosage and units
        dosageUnits du;
        du.dosage = aSingleRow[COLUMN_L];
        du.units = aSingleRow[COLUMN_M];
        duVec.push_back(du);
    }

    printFileStats(filename);
}

// Return count added
int getAdditionalNames(const std::string &rn,
                       std::set<std::string> &gtinUsedSet,
                       GTIN::oneFachinfoPackages &packages,
                       const std::string &language)
{
    // See RealExpertInfo.java:1544
    //  "a.H." --> "ev.nn.i.H."
    //  "p.c." --> "ev.ep.e.c."
    if (language == "fr")
        fromSwissmedic = "ev.ep.e.c.";

    std::set<std::string>::iterator it;
    int countAdded = 0;
    
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        std::string rn5 = regnrs[rowInt];
        if (rn5 != rn)
            continue;
        
        std::string g13 = gtin[rowInt];
        it = gtinUsedSet.find(g13);
        if (it == gtinUsedSet.end()) { // not found list of used GTINs, we must add the name
            countAdded++;
            statsAugmentedGtinCount++;
            statsTotalGtinCount++;

            std::string onePackageInfo;
#ifdef DEBUG_IDENTIFY_NAMES
            onePackageInfo += "swm+";
#endif
            onePackageInfo += theWholeSpreadSheet.at(rowInt).at(COLUMN_C);
            BEAUTY::beautifyName(onePackageInfo);
            // Verify presence of dosage
            std::regex r(R"(\d+)");
            if (!std::regex_search(onePackageInfo, r)) {
                statsRecoveredDosage++;
                //std::clog << "no dosage for " << name << std::endl;
                onePackageInfo += " " + duVec[rowInt].dosage;
                onePackageInfo += " " + duVec[rowInt].units;
            }

            std::string paf = BAG::getPricesAndFlags(g13, fromSwissmedic, categoryVec[rowInt]);
            if (!paf.empty())
                onePackageInfo += paf;

            gtinUsedSet.insert(g13);
            packages.gtin.push_back(g13);
            packages.name.push_back(onePackageInfo);
        }
    }
    
    if (countAdded > 0)
        statsAugmentedRegnCount++;

    return countAdded;
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
            app = theWholeSpreadSheet.at(rowInt).at(COLUMN_T) + " (Swissmedic)";
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

std::string getCategoryByGtin(const std::string &g)
{
    std::string cat;

    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++)
        if (gtin[rowInt] == g) {
            cat = categoryVec[rowInt];
            break;
        }

    return cat;
}

dosageUnits getByGtin(const std::string &g)
{
    dosageUnits du;

    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++)
        if (gtin[rowInt] == g) {
            du = duVec[rowInt];
            break;
        }

    return du;
}
    
}
