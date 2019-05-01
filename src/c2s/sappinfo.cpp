//
//  sappinfo.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 26 Apr 2019
//

#include <iostream>
#include <string>
#include <set>
#include <unordered_set>
#include <libgen.h>     // for basename()

#include <xlnt/xlnt.hpp>

#include "sappinfo.hpp"
#include "report.hpp"
#include "html_tags.h"

#define COLUMN_B        1   // Hauptindikation
#define COLUMN_C        2   // Indikation
#define COLUMN_G        6   // Wirkstoff
#define COLUMN_H        7   // Applikationsart
#define COLUMN_I        8   // max. verabreichte Tagesdosis
#define COLUMN_J        9   // Bemerkungen zur Dosierung
#define COLUMN_R       17   // ATC
#define COLUMN_U       20   // Filter

#define COLUMN_2_B      1   // Hauptindikation
#define COLUMN_2_C      2   // Indikation
#define COLUMN_2_G      6   // Wirkstoff
#define COLUMN_2_H      7   // Applikationsart
#define COLUMN_2_I      8   // max. verabreichte Tagesdosis 1. Trimenon
#define COLUMN_2_J      9   // max. verabreichte Tagesdosis 2. Trimenon
#define COLUMN_2_K     10   // max. verabreichte Tagesdosis 3. Trimenon
#define COLUMN_2_M     12   // Peripartale Dosierung
#define COLUMN_2_N     13   // Bemerkungen zur peripartalen Dosierung
#define COLUMN_2_Z     25   // ATC
#define COLUMN_2_AA    26   // SAPP-Monographie
#define COLUMN_2_AC    28   // Filter

#define FIRST_DATA_ROW_INDEX    1

//#define DEBUG_SAPPINFO

namespace SAPP
{
    // Parse-phase stats
    std::set<std::string> statsUniqueAtcSet;

    // Usage stats
    unsigned int statsBfByAtcFoundCount = 0;
    unsigned int statsBfByAtcNotFoundCount = 0;
    unsigned int statsPregnByAtcFoundCount = 0;
    unsigned int statsPregnByAtcNotFoundCount = 0;
    unsigned int statsTablesCount[2] = {0,0};

    std::string sheetTitle[2];
    std::vector< std::vector<std::string> > sheetBreastFeeding;
    std::vector< std::vector<std::string> > sheetPregnancy;
    
    std::vector<_breastfeed> breastFeedVec;
    std::vector<_pregnancy> pregnancyVec;
    
    // Common
#define TH_KEY_TYPE         "Application"

    // First sheet
#define TH_KEY_MAX_DAILY    "MaxDaily"
#define TH_KEY_COMMENT      "Comments"
    const std::vector<std::string> th_key = {
        TH_KEY_TYPE, TH_KEY_MAX_DAILY, TH_KEY_COMMENT
    };
    std::vector<std::string> th_de = {
        "Applikationsart", "max. verabreichte Tagesdosis", "Bemerkungen"
    };
    std::vector<std::string> th_fr = {        // TODO
        "Type of application", "max daily dose", "Comments"
    };
    std::vector<std::string> th_en = {
        "Type of application", "max daily dose", "Comments"
    };
    std::map<std::string, std::string> thTitleMap;

    // Second sheet
#define TH_KEY_MAX1                 "Max1"
#define TH_KEY_MAX2                 "Max2"
#define TH_KEY_MAX3                 "Max3"
#define TH_KEY_DOSE_ADJUST          "Adjustment"
#define TH_KEY_PERIDOSE             "periDosi"
#define TH_KEY_PERIDOSE_COMMENT     "periBemer"

    const std::vector<std::string> th_key_2 = {
        TH_KEY_TYPE, TH_KEY_MAX1, TH_KEY_MAX2, TH_KEY_MAX3,
        TH_KEY_DOSE_ADJUST, TH_KEY_PERIDOSE, TH_KEY_PERIDOSE_COMMENT
    };
    std::vector<std::string> th_de_2 = {
        "Applikationsart", "max TD Tr 1", "max TD Tr 2", "max TD Tr 3",
        "Dosisanpassung", "Peripartale Dosierung", "Bemerkungen zur peripartalen Dosierung"
    };
    std::vector<std::string> th_fr_2 = {        // TODO
        "Type of application", "max TD Tr 1", "max TD Tr 2", "max TD Tr 3",
        "Dose adjustment", "Peripartum dosage", "Comments on peripartum dosage"
    };
    std::vector<std::string> th_en_2 = {
        "Type of application", "max TD Tr 1", "max TD Tr 2", "max TD Tr 3",
        "Dose adjustment", "Peripartum dosage", "Comments on peripartum dosage"
    };
    std::map<std::string, std::string> thTitleMap_2;

    static void getBreastFeedByAtc(const std::string &atc, std::vector<_breastfeed> &bfv);
    static void getPregnancyByAtc(const std::string &atc, std::vector<_pregnancy> &pv);
    static void printFileStats(const std::string &filename);

static void printFileStats(const std::string &filename)
{
    REP::html_h2("Sappinfo (filtered)");
    //REP::html_p(std::string(basename((char *)filename.c_str())));
    REP::html_p(filename);
    REP::html_start_ul();
    REP::html_li("Unique ATC set: " + std::to_string(statsUniqueAtcSet.size()));
    REP::html_li("rows Breast Feeding: " + std::to_string(sheetBreastFeeding.size())); // TODO: localize
    REP::html_li("rows Pregnancy: " + std::to_string(sheetPregnancy.size())); // TODO: localize
    REP::html_end_ul();
}
    
void printUsageStats()
{
    REP::html_h2("SappInfo");

    REP::html_p("ATC (counting also repetitions)");
    REP::html_start_ul();
    REP::html_li("with <breastFeed>: " + std::to_string(statsBfByAtcFoundCount));
    REP::html_li("without <breastFeed>: " + std::to_string(statsBfByAtcNotFoundCount));
    REP::html_li("with <pregnancy>: " + std::to_string(statsPregnByAtcFoundCount));
    REP::html_li("without <pregnancy>: " + std::to_string(statsPregnByAtcNotFoundCount));
    REP::html_end_ul();

    REP::html_p("Other stats");
    REP::html_start_ul();
    REP::html_li("tables created: " +
                 std::to_string(statsTablesCount[0]) + " + " +
                 std::to_string(statsTablesCount[1]));
    REP::html_end_ul();
}

// TODO: use a set of ATCs to speed up the lookup
void parseXLXS(const std::string &filename,
               const std::string &language)
{
    const std::unordered_set<int> acceptedFiltersSet = { 1, 5, 6, 9 };

    // TODO: localization, see pedddose parseXML()
    {
        // Define localized lookup table for Sappinfo table header
        std::vector<std::string> &th = th_en;
        for (int i=0; i< th_key.size(); i++)
            thTitleMap.insert(std::make_pair(th_key[i], th[i]));

        std::vector<std::string> &th_2 = th_en_2;
        for (int i=0; i< th_key_2.size(); i++)
            thTitleMap_2.insert(std::make_pair(th_key_2[i], th_2[i]));

        if (language == "de") {
            th = th_de;
            th_2 = th_de_2;
        }
        else if (language == "fr") {
            th = th_fr;
            th_2 = th_fr_2;
        }
    }

    xlnt::workbook wb;
    wb.load(filename);
    //auto ws = wb.active_sheet();

    std::clog << std::endl << "Reading sappinfo XLSX" << std::endl;

    auto ws = wb.sheet_by_index(0);
    sheetTitle[0] = ws.title();
    std::clog << "\tSheet: " << ws.title() << std::endl;

    int skipHeaderCount = 0;
    for (auto row : ws.rows(false)) {
        
        if (++skipHeaderCount <= FIRST_DATA_ROW_INDEX) {
#ifdef DEBUG_SAPPINFO
            int i=0;
            for (auto cell : row)
                std::clog << i++ << "\t<" << cell.to_string() << ">" << std::endl;
#endif
            continue;
        }
        
        int filter = std::stoi(row[COLUMN_U].to_string());
        if (acceptedFiltersSet.find(filter) == acceptedFiltersSet.end())
            continue;            // Not found in set

        std::vector<std::string> aSingleRow;
        for (auto cell : row) {
            //std::clog << cell.to_string() << std::endl;
            aSingleRow.push_back(cell.to_string());
        }
        
        sheetBreastFeeding.push_back(aSingleRow);
        
        _breastfeed bf;
        bf.c.atcCode = aSingleRow[COLUMN_R];
        bf.c.activeSubstance = aSingleRow[COLUMN_G];
        bf.c.mainIndication = aSingleRow[COLUMN_B];
        bf.c.indication = aSingleRow[COLUMN_C];
        bf.c.typeOfApplication = aSingleRow[COLUMN_H];
        bf.maxDailyDose = aSingleRow[COLUMN_I];
        bf.comments = aSingleRow[COLUMN_J];
        breastFeedVec.push_back(bf);
        statsUniqueAtcSet.insert(bf.c.atcCode);
#ifdef DEBUG_SAPPINFO
        if (aSingleRow[COLUMN_R] == "J02AC01") {
            std::clog
            << "Art der Anwendung: " << ws.title()
            << "\n\t ATC: <" << aSingleRow[COLUMN_R] << ">"
            << "\n\t Wirkstoff: <" << aSingleRow[COLUMN_G] << ">"
            << "\n\t Hauptindikation: <" << aSingleRow[COLUMN_B] << ">"
            << "\n\t Indikation: <" << aSingleRow[COLUMN_C] << ">"
            << "\n\t Applikationsart: <" << aSingleRow[COLUMN_H] << ">"
            << "\n\t max: <" << aSingleRow[COLUMN_I] << ">"
            << "\n\t comment: <" << aSingleRow[COLUMN_J] << ">"
            << "\n\t Filter: <" << aSingleRow[COLUMN_U] << ">"
            << std::endl;
        }
#endif
    }
    
    // Pregnancy sheet
    
    ws = wb.sheet_by_index(1);
    sheetTitle[1] = ws.title();
    std::clog << "Sheet title: " << ws.title() << std::endl;
    
    skipHeaderCount = 0;
    for (auto row : ws.rows(false)) {
        if (++skipHeaderCount <= FIRST_DATA_ROW_INDEX) {
#ifdef DEBUG_SAPPINFO
            int i=0;
            for (auto cell : row)
                std::clog << i++ << "\t<" << cell.to_string() << ">" << std::endl;
#endif
            continue;
        }
        
        int filter = std::stoi(row[COLUMN_2_AC].to_string());
        if (acceptedFiltersSet.find(filter) == acceptedFiltersSet.end())
            continue;            // Not found in set
        
        std::vector<std::string> aSingleRow;
        for (auto cell : row) {
            //std::clog << cell.to_string() << std::endl;
            aSingleRow.push_back(cell.to_string());
        }
        
        sheetPregnancy.push_back(aSingleRow);
        
        _pregnancy pr;
        pr.c.atcCode = aSingleRow[COLUMN_2_Z];
        pr.c.activeSubstance = aSingleRow[COLUMN_2_G];
        pr.c.mainIndication = aSingleRow[COLUMN_2_B];
        pr.c.indication = aSingleRow[COLUMN_2_C];
        pr.c.typeOfApplication = aSingleRow[COLUMN_2_H];
        pr.link = aSingleRow[COLUMN_2_AA];
        if (pr.link == "nein")
            pr.link.clear();
        pr.max1 = aSingleRow[COLUMN_2_I];
        pr.max2 = aSingleRow[COLUMN_2_J];
        pr.max3 = aSingleRow[COLUMN_2_K];
        pr.periDosi = aSingleRow[COLUMN_2_M];
        pr.periBeme = aSingleRow[COLUMN_2_N];
        // TODO:
        pregnancyVec.push_back(pr);
        statsUniqueAtcSet.insert(pr.c.atcCode);
#ifdef DEBUG_SAPPINFO
        if (aSingleRow[COLUMN_2_Z] == "J01FA01") {
            std::clog
            << "Art der Anwendung: " << ws.title()
            << "\n\t ATC: <" << aSingleRow[COLUMN_2_Z] << ">"
            << "\n\t Wirkstoff: <" << aSingleRow[COLUMN_2_G] << ">"
            << "\n\t Hauptindikation: <" << aSingleRow[COLUMN_2_B] << ">"
            << "\n\t Indikation: <" << aSingleRow[COLUMN_2_C] << ">"
            << "\n\t Applikationsart: <" << aSingleRow[COLUMN_2_H] << ">"
            << "\n\t SAPP-Monographie: <" << aSingleRow[COLUMN_2_AA] << ">"
            << "\n\t max TD Tr 1: <" << aSingleRow[COLUMN_2_I] << ">"
            << "\n\t max TD Tr 2: <" << aSingleRow[COLUMN_2_J] << ">"
            << "\n\t max TD Tr 3: <" << aSingleRow[COLUMN_2_K] << ">"
            << "\n\t Peripartale Dosierung: <" << aSingleRow[COLUMN_2_M] << ">"
            << "\n\t Bemerkungen zur peripartalen Dosierung: <" << aSingleRow[COLUMN_2_N] << ">"
            << "\n\t Filter: <" << aSingleRow[COLUMN_2_AC] << ">"
            << std::endl;
        }
#endif
    }

    printFileStats(filename);
}
    
// There could be multiple lines for the same ATC. Return a vector
static void getBreastFeedByAtc(const std::string &atc, std::vector<_breastfeed> &bfv)
{
    for (auto b : breastFeedVec) {
        if (b.c.atcCode == atc)
            bfv.push_back(b);
    }
}
    
// There could be multiple lines for the same ATC. Return a vector
static void getPregnancyByAtc(const std::string &atc, std::vector<_pregnancy> &pv)
{
    for (auto p : pregnancyVec) {
        if (p.c.atcCode == atc)
            pv.push_back(p);
    }
}

std::string getHtmlByAtc(const std::string atc)
{
    //std::clog << basename((char *)__FILE__) << ":" << __LINE__ << " " << atc << std::endl;
    
    if (atc.empty())
        return {};
    
    // First check ordered set, quicker than going through the whole vector
    if (statsUniqueAtcSet.find(atc) == statsUniqueAtcSet.end())
        return {};

    std::string html;
    html.clear();

    //---
    std::vector<_breastfeed> bfv;
    getBreastFeedByAtc(atc, bfv);

    if (bfv.empty())
        statsBfByAtcNotFoundCount++;
    else
        statsBfByAtcFoundCount++;
    
    for (auto b : bfv) {
#if 1
        // Check for optional columns
        std::map<std::string, bool> optionalColumnMap = {
            {TH_KEY_COMMENT, false}
        };
        int numColumns = th_key.size() - optionalColumnMap.size();
        if (!optionalColumnMap[TH_KEY_COMMENT] &&
            !b.comments.empty())
        {
            optionalColumnMap[TH_KEY_COMMENT] = true;
            numColumns++;
        }
#endif

        // Start defining the HTML code
        std::string textBeforeTable;
        {
            textBeforeTable += "Art der Anwendung: " + sheetTitle[0] + "<br />\n";
            textBeforeTable += "ATC-Code: " + b.c.atcCode + "<br />\n";
            textBeforeTable += "Wirkstoff: " + b.c.activeSubstance + "<br />\n"; // TODO: localize
            if (!b.c.mainIndication.empty())
                textBeforeTable += "Hauptindikation: " + b.c.mainIndication + "<br />\n"; // TODO: localize

            if (!b.c.indication.empty())
                textBeforeTable += "Indikation: " + b.c.indication + "<br />\n"; // TODO: localize
        }
        html += "\n<p class=\"spacing1\">" + textBeforeTable + "</p>\n";

        std::string tableColGroup("<col span=\"" + std::to_string(numColumns) + "\" style=\"background-color: #EEEEEE; padding-right: 5px; padding-left: 5px\"/>");

        tableColGroup = "<colgroup>" + tableColGroup + "</colgroup>";
        
        std::string tableHeader;
        tableHeader.clear();

        std::string tableBody;
        tableBody.clear();
        
        {
            tableHeader += TAG_TH_L + thTitleMap[TH_KEY_TYPE] + TAG_TH_R;
            tableHeader += TAG_TH_L + thTitleMap[TH_KEY_MAX_DAILY] + TAG_TH_R;
            if (optionalColumnMap[TH_KEY_COMMENT])
                tableHeader += TAG_TH_L + thTitleMap[TH_KEY_COMMENT] + TAG_TH_R;

            tableHeader += "\n"; // for readability
            tableHeader = "<tr>" + tableHeader + "</tr>";
#ifdef WITH_SEPARATE_TABLE_HEADER
            tableHeader = "<thead>" + tableHeader + "</thead>";
#else
            tableBody += tableHeader;
#endif
        }

        {
            std::string tableRow;

            tableRow += TAG_TD_L + b.c.typeOfApplication + TAG_TD_R;
            tableRow += TAG_TD_L + b.maxDailyDose + TAG_TD_R;
            if (optionalColumnMap[TH_KEY_COMMENT])
                tableRow += TAG_TD_L + b.comments + TAG_TD_R;

            tableRow += "\n";  // for readability
            tableRow = "<tr>" + tableRow + "</tr>";
            tableBody += tableRow;
        }

        tableBody = "<tbody>" + tableBody + "</tbody>";

        std::string table = tableColGroup;
#ifdef WITH_SEPARATE_TABLE_HEADER
        table += tableHeader + tableBody;
#else
        table += tableBody;
#endif
        table = TAG_TABLE_L + table + TAG_TABLE_R;
        html += table;

        statsTablesCount[0]++;
    }  // for bfv

    //---
    std::vector<_pregnancy> pregnv;
    getPregnancyByAtc(atc, pregnv);

    if (pregnv.empty())
        statsPregnByAtcNotFoundCount++;
    else
        statsPregnByAtcFoundCount++;
    
    for (auto p : pregnv) {
#if 1
        // Check for optional columns
        std::map<std::string, bool> optionalColumnMap = {
            {TH_KEY_MAX1, false},
            {TH_KEY_MAX2, false},
            {TH_KEY_MAX3, false}
        };
        int numColumns = th_key_2.size() - optionalColumnMap.size();
        if (!optionalColumnMap[TH_KEY_MAX1] &&
            !p.max1.empty())
        {
            optionalColumnMap[TH_KEY_MAX1] = true;
            numColumns++;
        }

        if (!optionalColumnMap[TH_KEY_MAX2] &&
            !p.max2.empty())
        {
            optionalColumnMap[TH_KEY_MAX2] = true;
            numColumns++;
        }

        if (!optionalColumnMap[TH_KEY_MAX3] &&
            !p.max3.empty())
        {
            optionalColumnMap[TH_KEY_MAX3] = true;
            numColumns++;
        }
#else
        // TODO: Check for optional columns
        int numColumns = th_key_2.size();
#endif
        
        // Define the HTML code
        std::string textBeforeTable;
        {
            textBeforeTable += "Art der Anwendung: " + sheetTitle[1] + "<br />\n";
            textBeforeTable += "ATC-Code: " + p.c.atcCode + "<br />\n";
            textBeforeTable += "Wirkstoff: " + p.c.activeSubstance + "<br />\n"; // TODO: localize
            if (!p.c.mainIndication.empty())
                textBeforeTable += "Hauptindikation: " + p.c.mainIndication + "<br />\n"; // TODO: localize
            
            if (!p.c.indication.empty())
                textBeforeTable += "Indikation: " + p.c.indication + "<br />\n"; // TODO: localize
            
#if 0
            if (!p.link.empty())
                textBeforeTable += "Sappinfo Monographie (Link) " + p.link + "<br />\n"; // TODO: localize
#else
            if (!p.link.empty())
                textBeforeTable += "<a href=\"" + p.link + "\">Sappinfo Monographie (Link)</a>" + "<br />\n"; // TODO: localize
#endif
        }
        html += "\n<p class=\"spacing1\">" + textBeforeTable + "</p>\n";
        
        std::string tableColGroup("<col span=\"" + std::to_string(numColumns) + "\" style=\"background-color: #EEEEEE; padding-right: 5px; padding-left: 5px\"/>");
        
        tableColGroup = "<colgroup>" + tableColGroup + "</colgroup>";
        
        std::string tableHeader;
        tableHeader.clear();
        
        std::string tableBody;
        tableBody.clear();
        
        {
            tableHeader += TAG_TH_L + thTitleMap_2[TH_KEY_TYPE] + TAG_TH_R;
            if (optionalColumnMap[TH_KEY_MAX1])
                tableHeader += TAG_TH_L + thTitleMap_2[TH_KEY_MAX1] + TAG_TH_R;

            if (optionalColumnMap[TH_KEY_MAX2])
                tableHeader += TAG_TH_L + thTitleMap_2[TH_KEY_MAX2] + TAG_TH_R;

            if (optionalColumnMap[TH_KEY_MAX3])
                tableHeader += TAG_TH_L + thTitleMap_2[TH_KEY_MAX3] + TAG_TH_R;

            tableHeader += TAG_TH_L + thTitleMap_2[TH_KEY_DOSE_ADJUST] + TAG_TH_R;
            tableHeader += TAG_TH_L + thTitleMap_2[TH_KEY_PERIDOSE] + TAG_TH_R;
            tableHeader += TAG_TH_L + thTitleMap_2[TH_KEY_PERIDOSE_COMMENT] + TAG_TH_R;
            // TODO: optional columns
            
            tableHeader += "\n"; // for readability
            tableHeader = "<tr>" + tableHeader + "</tr>";
#ifdef WITH_SEPARATE_TABLE_HEADER
            tableHeader = "<thead>" + tableHeader + "</thead>";
#else
            tableBody += tableHeader;
#endif
        }
        
        {
            std::string tableRow;
            
            tableRow += TAG_TD_L + p.c.typeOfApplication + TAG_TD_R;
            if (optionalColumnMap[TH_KEY_MAX1])
                tableRow += TAG_TD_L + p.max1 + TAG_TD_R;

            if (optionalColumnMap[TH_KEY_MAX2])
                tableRow += TAG_TD_L + p.max2 + TAG_TD_R;

            if (optionalColumnMap[TH_KEY_MAX3])
                tableRow += TAG_TD_L + p.max3 + TAG_TD_R;

            tableRow += TAG_TD_L + std::string("???") + TAG_TD_R; // TODO
            tableRow += TAG_TD_L + p.periDosi + TAG_TD_R;
            tableRow += TAG_TD_L + p.periBeme + TAG_TD_R;
            // TODO:
            
            tableRow += "\n";  // for readability
            tableRow = "<tr>" + tableRow + "</tr>";
            tableBody += tableRow;
        }
        
        tableBody = "<tbody>" + tableBody + "</tbody>";
        
        std::string table = tableColGroup;
#ifdef WITH_SEPARATE_TABLE_HEADER
        table += tableHeader + tableBody;
#else
        table += tableBody;
#endif
        table = TAG_TABLE_L + table + TAG_TABLE_R;
        html += table;
        
        statsTablesCount[1]++;
    } // for pregnv

    return html;
}

}
