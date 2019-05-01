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
#include <boost/algorithm/string.hpp>

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
    
    ////////////////////////////////////////////////////////////////////////////
    // LOCALIZATION

    // Common
#define LOC_KEY_TYPE                    "type"
#define LOC_KEY_ACT_SUBST               "active"
#define LOC_KEY_MAIN_INDIC              "mainInd"
#define LOC_KEY_INDICATION              "indication"

#define LOC_KEY_TH_TYPE                 "application"

    // First sheet
#define LOC_KEY_TH_MAX_DAILY            "MaxDaily"
#define LOC_KEY_TH_COMMENT              "Comments"
    
    // Second sheet
#define LOC_KEY_TH_MAX1                 "Max1"
#define LOC_KEY_TH_MAX2                 "Max2"
#define LOC_KEY_TH_MAX3                 "Max3"
#define LOC_KEY_TH_DOSE_ADJUST          "Adjustment"
#define LOC_KEY_TH_PERIDOSE             "periDosi"
#define LOC_KEY_TH_PERIDOSE_COMMENT     "periBemer"

    const std::vector<std::string> loc_string_key = {
        LOC_KEY_TH_TYPE,
        
        LOC_KEY_TH_MAX_DAILY, LOC_KEY_TH_COMMENT,
        
        LOC_KEY_TH_MAX1, LOC_KEY_TH_MAX2, LOC_KEY_TH_MAX3,
        LOC_KEY_TH_DOSE_ADJUST, LOC_KEY_TH_PERIDOSE, LOC_KEY_TH_PERIDOSE_COMMENT,
        
        LOC_KEY_TYPE, LOC_KEY_ACT_SUBST, LOC_KEY_MAIN_INDIC, LOC_KEY_INDICATION
    };
    std::vector<std::string> loc_string_de = {
        "Applikationsart",
        
        "max. verabreichte Tagesdosis", "Bemerkungen",
        
        "max TD Tr 1", "max TD Tr 2", "max TD Tr 3",
        "Dosisanpassung", "Peripartale Dosierung", "Bemerkungen zur peripartalen Dosierung",
        
        "Art der Anwendung", "Wirkstoff", "Hauptindikation", "Indikation"
    };
    std::vector<std::string> loc_string_fr = { // TODO: verify translations
        "Type d’application",
        
        "dose quotidienne max", "Comments",
        
        "max TD Tr 1", "max TD Tr 2", "max TD Tr 3",
        "Ajustement de la dose", "Peripartum posologie", "Commentaires sur périnatale posologie",
        
        "Type d'utilisation", "Substance active", "Indication principale", "Indication"
    };
    std::vector<std::string> loc_string_en = {
        "Type of application",
        
        "max daily dose", "Comments",

        "max TD Tr 1", "max TD Tr 2", "max TD Tr 3",
        "Dose adjustment", "Peripartum dosage", "Comments on peripartum dosage",
        
        "Type of use", "Active Substance", "Main Indication", "Indication"
    };
    std::map<std::string, std::string> localizedResourcesMap;

    ////////////////////////////////////////////////////////////////////////////

    // First sheet
    const std::vector<std::string> requiredColumnVec = {
        LOC_KEY_TH_TYPE, LOC_KEY_TH_MAX_DAILY
    };
    std::map<std::string, bool> optionalColumnMap = {
        {LOC_KEY_TH_COMMENT, false}
    };

    // Second sheet
    const std::vector<std::string> requiredColumnVec_2 = {
        LOC_KEY_TH_TYPE, LOC_KEY_TH_DOSE_ADJUST, LOC_KEY_TH_PERIDOSE
    };
    std::map<std::string, bool> optionalColumnMap_2 = {
        {LOC_KEY_TH_MAX1, false},
        {LOC_KEY_TH_MAX2, false},
        {LOC_KEY_TH_MAX3, false},
        {LOC_KEY_TH_PERIDOSE_COMMENT, false}
    };
    
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

    {
        // Define localized strings
        std::vector<std::string> &loc_string = loc_string_en;

        if (language == "de") {
            loc_string = loc_string_de;
        }
        else if (language == "fr") {
            loc_string = loc_string_fr;
        }

        for (int i=0; i< loc_string_key.size(); i++)
            localizedResourcesMap.insert(std::make_pair(loc_string_key[i], loc_string[i]));
    }

    xlnt::workbook wb;
    wb.load(filename);
    //auto ws = wb.active_sheet();

    std::clog << std::endl << "Reading sappinfo XLSX" << std::endl;

    // Breast-feeding sheet
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
        bf.c.atcCodes = aSingleRow[COLUMN_R];
#if 1 // issue 53
        // Also break it down into single ATCs
        boost::algorithm::split(bf.c.atcCodeVec, bf.c.atcCodes, boost::is_any_of(ATC_LIST_SEPARATOR), boost::token_compress_on);

        for (auto a : bf.c.atcCodeVec)
            statsUniqueAtcSet.insert(a);
#endif
        bf.c.activeSubstance = aSingleRow[COLUMN_G];
        bf.c.mainIndication = aSingleRow[COLUMN_B];
        bf.c.indication = aSingleRow[COLUMN_C];
        bf.c.typeOfApplication = aSingleRow[COLUMN_H];
        bf.maxDailyDose = aSingleRow[COLUMN_I];
        bf.comments = aSingleRow[COLUMN_J];
        breastFeedVec.push_back(bf);
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
        pr.c.atcCodes = aSingleRow[COLUMN_2_Z];
#if 1 // issue 53
        // Also break it down into single ATCs
        boost::algorithm::split(pr.c.atcCodeVec, pr.c.atcCodes, boost::is_any_of(ATC_LIST_SEPARATOR), boost::token_compress_on);
        
        for (auto a : pr.c.atcCodeVec)
            statsUniqueAtcSet.insert(a);
#endif
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
        pregnancyVec.push_back(pr);
#ifdef DEBUG_SAPPINFO
        if (aSingleRow[COLUMN_2_Z] == "J01FA01")
        {
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
template <class T>
void getByAtc(const std::string &atc,
              const std::vector<T> &inVec,
              std::vector<T> &outVec)
{
    for (auto item : inVec)
        for (auto a : item.c.atcCodeVec)
            if (a == atc) {
                outVec.push_back(item);
                
                // Break out of the inner loop, to move onto the next item
                // Not a big speed gain for only two items, but logically it makes sense
                break;
            }
}

static void getBreastFeedByAtc(const std::string &atc, std::vector<_breastfeed> &bfv)
{
    getByAtc<_breastfeed>(atc, breastFeedVec, bfv);
}
    
static void getPregnancyByAtc(const std::string &atc, std::vector<_pregnancy> &pv)
{
    getByAtc<_pregnancy>(atc, pregnancyVec, pv);
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
        int numColumns = requiredColumnVec.size();
        for (auto &m : optionalColumnMap)  // reset options
            m.second = false;

        if (!optionalColumnMap[LOC_KEY_TH_COMMENT] &&
            !b.comments.empty())
        {
            optionalColumnMap[LOC_KEY_TH_COMMENT] = true;
            numColumns++;
        }
#endif

        // Start defining the HTML code
        std::string textBeforeTable;
        {
            textBeforeTable += localizedResourcesMap[LOC_KEY_TYPE] + ": " + sheetTitle[0] + "<br />\n";
            textBeforeTable += "ATC-Code: " + b.c.atcCodes + "<br />\n";
            textBeforeTable += localizedResourcesMap[LOC_KEY_ACT_SUBST] + ": " + b.c.activeSubstance + "<br />\n";
            if (!b.c.mainIndication.empty())
                textBeforeTable += localizedResourcesMap[LOC_KEY_MAIN_INDIC] + ": " + b.c.mainIndication + "<br />\n";

            if (!b.c.indication.empty())
                textBeforeTable += localizedResourcesMap[LOC_KEY_INDICATION] + ": " + b.c.indication + "<br />\n";
        }
        html += "\n<p class=\"spacing1\">" + textBeforeTable + "</p>\n";

        std::string tableColGroup(COL_SPAN_L + std::to_string(numColumns) + COL_SPAN_R);
        tableColGroup = "<colgroup>" + tableColGroup + "</colgroup>";
        
        std::string tableHeader;
        tableHeader.clear();

        std::string tableBody;
        tableBody.clear();
        
        {
            tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_TYPE] + TAG_TH_R;
            tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_MAX_DAILY] + TAG_TH_R;

            if (optionalColumnMap[LOC_KEY_TH_COMMENT])
                tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_COMMENT] + TAG_TH_R;

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
            if (optionalColumnMap[LOC_KEY_TH_COMMENT])
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
        int numColumns = requiredColumnVec_2.size();
        for (auto &m : optionalColumnMap_2)  // reset options
            m.second = false;

        if (!optionalColumnMap_2[LOC_KEY_TH_MAX1] &&
            !p.max1.empty())
        {
            optionalColumnMap_2[LOC_KEY_TH_MAX1] = true;
            numColumns++;
        }

        if (!optionalColumnMap_2[LOC_KEY_TH_MAX2] &&
            !p.max2.empty())
        {
            optionalColumnMap_2[LOC_KEY_TH_MAX2] = true;
            numColumns++;
        }

        if (!optionalColumnMap_2[LOC_KEY_TH_MAX3] &&
            !p.max3.empty())
        {
            optionalColumnMap_2[LOC_KEY_TH_MAX3] = true;
            numColumns++;
        }

        if (!optionalColumnMap_2[LOC_KEY_TH_PERIDOSE_COMMENT] &&
            !p.periBeme.empty())
        {
            optionalColumnMap_2[LOC_KEY_TH_PERIDOSE_COMMENT] = true;
            numColumns++;
        }
#endif
        
        // Define the HTML code
        std::string textBeforeTable;
        {
            textBeforeTable += localizedResourcesMap[LOC_KEY_TYPE] + ": " + sheetTitle[1] + "<br />\n";
            textBeforeTable += "ATC-Code: " + p.c.atcCodes + "<br />\n";
            textBeforeTable += localizedResourcesMap[LOC_KEY_ACT_SUBST] + ": " + p.c.activeSubstance + "<br />\n";
            if (!p.c.mainIndication.empty())
                textBeforeTable += localizedResourcesMap[LOC_KEY_MAIN_INDIC] + ": " + p.c.mainIndication + "<br />\n";
            
            if (!p.c.indication.empty())
                textBeforeTable += localizedResourcesMap[LOC_KEY_INDICATION] + ": " + p.c.indication + "<br />\n";
            
            if (!p.link.empty())
                textBeforeTable += "<a href=\"" + p.link + "\">Sappinfo Monographie (Link)</a>" + "<br />\n"; // TODO: localize
        }
        html += "\n<p class=\"spacing1\">" + textBeforeTable + "</p>\n";
        
        std::string tableColGroup(COL_SPAN_L + std::to_string(numColumns) + COL_SPAN_R);
        tableColGroup = "<colgroup>" + tableColGroup + "</colgroup>";
        
        std::string tableHeader;
        tableHeader.clear();
        
        std::string tableBody;
        tableBody.clear();
        
        {
            tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_TYPE] + TAG_TH_R;

            if (optionalColumnMap_2[LOC_KEY_TH_MAX1])
                tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_MAX1] + TAG_TH_R;

            if (optionalColumnMap_2[LOC_KEY_TH_MAX2])
                tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_MAX2] + TAG_TH_R;

            if (optionalColumnMap_2[LOC_KEY_TH_MAX3])
                tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_MAX3] + TAG_TH_R;

            tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_DOSE_ADJUST] + TAG_TH_R;
            tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_PERIDOSE] + TAG_TH_R;

            if (optionalColumnMap_2[LOC_KEY_TH_PERIDOSE_COMMENT])
                tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_PERIDOSE_COMMENT] + TAG_TH_R;
            
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
            if (optionalColumnMap_2[LOC_KEY_TH_MAX1])
                tableRow += TAG_TD_L + p.max1 + TAG_TD_R;

            if (optionalColumnMap_2[LOC_KEY_TH_MAX2])
                tableRow += TAG_TD_L + p.max2 + TAG_TD_R;

            if (optionalColumnMap_2[LOC_KEY_TH_MAX3])
                tableRow += TAG_TD_L + p.max3 + TAG_TD_R;

            tableRow += TAG_TD_L + std::string("???") + TAG_TD_R; // TODO
            tableRow += TAG_TD_L + p.periDosi + TAG_TD_R;

            if (optionalColumnMap_2[LOC_KEY_TH_PERIDOSE_COMMENT])
                tableRow += TAG_TD_L + p.periBeme + TAG_TD_R;
            
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
