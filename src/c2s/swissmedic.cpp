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
#include <unordered_map>
#include <boost/algorithm/string.hpp>

#include <xlnt/xlnt.hpp>

#include "swissmedic.hpp"
#include "gtin.hpp"
#include "bag.hpp"
#include "bagFHIR.hpp"
#include "refdata.hpp"
#include "beautify.hpp"
#include "report.hpp"

#define COLUMN_A        0   // GTIN (5 digits)
#define COLUMN_C        2   // name
#define COLUMN_D        3   // auth, Zulassungsinhaberin
#define COLUMN_G        6   // ATC
#define COLUMN_K       10   // packaging code (3 digits)
#define COLUMN_L       11   // number for dosage
#define COLUMN_M       12   // units for dosage
#define COLUMN_N       13   // category (A..E)
#define COLUMN_T       19   // application field
#define COLUMN_W       22   // preparation contains narcotics

#define FIRST_DATA_ROW_INDEX    6

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

    // Indexes into theWholeSpreadSheet, built once at the end of parseXLXS().
    // Every lookup below used to be a linear scan over ~18'000 rows, run once
    // per registration number and once per package of every Fachinfo.
    std::unordered_map<std::string, std::vector<int>> rowsByRegnr;
    std::unordered_map<std::string, int> rowByGtin;    // first row with that GTIN
    std::unordered_map<std::string, int> rowByGtin12;  // ... without the checksum

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

    // Build the lookup indexes. emplace() keeps the first row for a given GTIN,
    // which is what the "first match wins" scans below used to return.
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        rowsByRegnr[regnrs[rowInt]].push_back(rowInt);
        rowByGtin.emplace(gtin[rowInt], rowInt);
        rowByGtin12.emplace(gtin[rowInt].substr(0, 12), rowInt);
    }

    printFileStats(filename);
}

// Return count added
int getAdditionalNames(const std::string &rn,
                       std::set<std::string> &gtinUsedSet,
                       GTIN::oneFachinfoPackages &packages,
                       const std::string &language,
                       bool fhir)
{
    // See RealExpertInfo.java:1544
    //  "a.H." --> "ev.nn.i.H."
    //  "p.c." --> "ev.ep.e.c."
    if (language == "fr")
        fromSwissmedic = "ev.ep.e.c.";

    std::set<std::string>::iterator it;
    int countAdded = 0;

    auto rows = rowsByRegnr.find(rn);
    if (rows == rowsByRegnr.end())
        return 0;

    for (int rowInt : rows->second) {
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

            std::string paf = fhir ? BAGFHIR::getPricesAndFlags(g13, fromSwissmedic, categoryVec[rowInt])
                                   : BAG::getPricesAndFlags(g13, fromSwissmedic, categoryVec[rowInt]);
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
    auto rows = rowsByRegnr.find(rn);

    return rows == rowsByRegnr.end() ? 0 : static_cast<int>(rows->second.size());
}

bool findGtin(const std::string &gtin)
{
    // The comparison is only the first 12 digits, without checksum
    return rowByGtin12.find(gtin.substr(0,12)) != rowByGtin12.end();
}

std::string getApplication(const std::string &rn)
{
    std::string app;

    auto rows = rowsByRegnr.find(rn);
    if (rows != rowsByRegnr.end())
        app = theWholeSpreadSheet.at(rows->second.front()).at(COLUMN_T) + " (Swissmedic)";

    return app;
}

std::string getAtcFromFirstRn(const std::string &rn)
{
    std::string atc;

    auto rows = rowsByRegnr.find(rn);
    if (rows != rowsByRegnr.end())
        atc = theWholeSpreadSheet.at(rows->second.front()).at(COLUMN_G);

    return atc;
}

std::string getCategoryByGtin(const std::string &g)
{
    std::string cat;

    auto row = rowByGtin.find(g);
    if (row != rowByGtin.end())
        cat = categoryVec[row->second];

    return cat;
}

dosageUnits getByGtin(const std::string &g)
{
    dosageUnits du;

    auto row = rowByGtin.find(g);
    if (row != rowByGtin.end())
        du = duVec[row->second];

    return du;
}

std::vector<std::string> getRegnrs() {
    return regnrs;
}

DB::RowToInsert getRow(const std::string &regnr) {
    DB::RowToInsert result;
    result.regnrs = regnr;

    auto rows = rowsByRegnr.find(regnr);
    if (rows != rowsByRegnr.end()) {
        int rowInt = rows->second.front();
        auto thisRow = theWholeSpreadSheet.at(rowInt);
        std::string rn5 = regnrs[rowInt];

        std::string name = REFDATA::findName(rn5);
        if (name.empty()) {
            std::string packageName = thisRow.at(COLUMN_C);
            std::string::size_type len = packageName.find(",");
            name = len != packageName.npos ? packageName.substr(0, len) : packageName;  // pos, len
        }

        result.title = name;
        result.auth = thisRow.at(COLUMN_D);
        result.atc = thisRow.at(COLUMN_G);
    }
    return result;
}

}
