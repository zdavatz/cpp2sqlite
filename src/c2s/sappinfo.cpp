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

namespace SAPP
{
    // Parse-phase stats
    std::set<std::string> statsUniqueAtcSet;

    // Usage stats
    unsigned int statsBfByAtcFoundCount = 0;
    unsigned int statsBfByAtcNotFoundCount = 0;
    unsigned int statsPregnByAtcFoundCount = 0;
    unsigned int statsPregnByAtcNotFoundCount = 0;
    unsigned int statsTablesCount = 0;

    std::vector< std::vector<std::string> > sheetBreastFeeding;
    std::vector< std::vector<std::string> > sheetPregnancy;
    
    std::vector<_breastfeed> breastFeedVec;
    std::vector<_pregnancy> pregnancyVec;
    
#define TH_KEY_TYPE      "Application"

    const std::vector<std::string> th_key = {
        TH_KEY_TYPE
    };
    std::vector<std::string> th_de = {
        "Applikationsart"
    };
    std::vector<std::string> th_fr = {
        "Type of application"  // TODO
    };
    std::vector<std::string> th_en = {
        "Type of application"
    };
    std::map<std::string, std::string> thTitleMap;

static
void printFileStats(const std::string &filename)
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
    REP::html_li("tables created: " + std::to_string(statsTablesCount));
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
        if (language == "de") {
            th = th_de;
        }
        else if (language == "fr") {
            th = th_fr;
        }

        for (int i=0; i< th_key.size(); i++)
            thTitleMap.insert(std::make_pair(th_key[i], th[i]));
    }

    xlnt::workbook wb;
    wb.load(filename);
    //auto ws = wb.active_sheet();

    std::clog << std::endl << "Reading sappinfo XLSX" << std::endl;

    auto ws = wb.sheet_by_index(0);
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
        breastFeedVec.push_back(bf);
        statsUniqueAtcSet.insert(bf.c.atcCode);
#ifdef DEBUG_SAPPINFO
        if (aSingleRow[COLUMN_R] == "J02AC01") {
            std::clog
            << "ATC: <" << aSingleRow[COLUMN_R] << ">"
            << ", Wirkstoff: <" << aSingleRow[COLUMN_G] << ">"
            << ", Hauptindikation: <" << aSingleRow[COLUMN_B] << ">"
            << ", Indikation: <" << aSingleRow[COLUMN_C] << ">"
            << ", Applikationsart: <" << aSingleRow[COLUMN_H] << ">"
            << ", max: <" << aSingleRow[COLUMN_I] << ">"
            << ", Filter: <" << aSingleRow[COLUMN_U] << ">"
            << std::endl;
        }
#endif
    }
    
    // Pregnancy sheet
    
    ws = wb.sheet_by_index(1);
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
            << "\n\t SAPP-Monographie: <" << aSingleRow[COLUMN_2_AA] << ">"
            << "\n\t Applikationsart: <" << aSingleRow[COLUMN_2_H] << ">"
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
static
void getBreastFeedByAtc(const std::string &atc, std::vector<_breastfeed> &bfv)
{
    for (auto b : breastFeedVec) {
        if (b.c.atcCode == atc)
            bfv.push_back(b);
    }
}
    
// There could be multiple lines for the same ATC. Return a vector
static
void getPregnancyByAtc(const std::string &atc, std::vector<_pregnancy> &pv)
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

    // TODO: handle the case where we are called multiple times with the same ATC B05AA01 R06AE07 A01AB03

    std::string html;
    html.clear();

    //---
    std::vector<_breastfeed> bfv;
    getBreastFeedByAtc(atc, bfv);

    if (bfv.empty()) {
        statsBfByAtcNotFoundCount++;
    }
    
    statsBfByAtcFoundCount++;
    
    for (auto b : bfv) {

        int numColumns = th_key.size();

        // Start defining the HTML code
        std::string textBeforeTable;
        {
            textBeforeTable += "ATC-Code: " + b.c.atcCode + "<br />\n";
            textBeforeTable += "Wirkstoff: " + b.c.activeSubstance + "<br />\n"; // TODO: localize
            // TODO
        }
        html += "\n<p class=\"spacing1\">" + textBeforeTable + "</p>\n";

        std::string tableColGroup("<col span=\"" + std::to_string(numColumns) + "\" style=\"background-color: #EEEEEE; padding-right: 5px; padding-left: 5px\"/>");

        tableColGroup = "<colgroup>" + tableColGroup + "</colgroup>";
        
        std::string tableHeader;
        tableHeader.clear();

        std::string tableBody;
        tableBody.clear();
        
        tableHeader += TAG_TH_L + thTitleMap[TH_KEY_TYPE] + TAG_TH_R;
        // TODO

        tableBody = "<tbody>" + tableBody + "</tbody>";

        std::string table = tableColGroup;
        table += tableBody;
        table = TAG_TABLE_L + table + TAG_TABLE_R;
        html += table;

        statsTablesCount++;
    }  // for bfv

    //---
    std::vector<_pregnancy> pregnv;
    getPregnancyByAtc(atc, pregnv);

    if (pregnv.empty()) {
        statsPregnByAtcNotFoundCount++;
    }

    statsPregnByAtcFoundCount++;

    return html;
}

}
