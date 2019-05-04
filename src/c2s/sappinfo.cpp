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
#include <map>
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
#define COLUMN_Q       16   // Zulassungsnummer
#define COLUMN_R       17   // ATC
#define COLUMN_S       18   // SAPP-Monographie
#define COLUMN_U       20   // Filter

#define COLUMN_2_B      1   // Hauptindikation
#define COLUMN_2_C      2   // Indikation
#define COLUMN_2_G      6   // Wirkstoff
#define COLUMN_2_H      7   // Applikationsart
#define COLUMN_2_I      8   // max. verabreichte Tagesdosis 1. Trimenon
#define COLUMN_2_J      9   // max. verabreichte Tagesdosis 2. Trimenon
#define COLUMN_2_K     10   // max. verabreichte Tagesdosis 3. Trimenon
#define COLUMN_2_L     11   // Bemerkungen zur Dosierung
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
#define LOC_KEY_TH_TYPE                 "appl" // application
#define LOC_KEY_TH_COMMENT              "comm" // comments

    // Sheet 1
#define LOC_KEY_TH_MAX_DAILY            "maxD" // maxDaily
#define LOC_KEY_TH_APPROVAL             "appr" // approval

    // Sheet 2
#define LOC_KEY_TH_MAX1                 "Max1"
#define LOC_KEY_TH_MAX2                 "Max2"
#define LOC_KEY_TH_MAX3                 "Max3"
#define LOC_KEY_TH_PERIDOSE             "perD" // periDosi
#define LOC_KEY_TH_PERIDOSE_COMMENT     "perB" // periBemer

    // Other strings
#define LOC_KEY_TYPE                    "type"
#define LOC_KEY_ACT_SUBST               "actv" // active
#define LOC_KEY_MAIN_INDIC              "mInd" // mainIndication
#define LOC_KEY_INDICATION              "indc" // indication
#define LOC_KEY_SHEET1                  "brFd" // breastFeed
#define LOC_KEY_SHEET2                  "preg" // pregnancy

    const std::vector<std::string> loc_string_key = {
        LOC_KEY_TH_TYPE, LOC_KEY_TH_COMMENT,
        
        // Sheet 1
        LOC_KEY_TH_MAX_DAILY, LOC_KEY_TH_APPROVAL,
        
        // Sheet 2
        LOC_KEY_TH_MAX1, LOC_KEY_TH_MAX2, LOC_KEY_TH_MAX3,
        LOC_KEY_TH_PERIDOSE, LOC_KEY_TH_PERIDOSE_COMMENT,
        
        // Other strings
        LOC_KEY_TYPE, LOC_KEY_ACT_SUBST, LOC_KEY_MAIN_INDIC, LOC_KEY_INDICATION,
        LOC_KEY_SHEET1, LOC_KEY_SHEET2
    };
    std::vector<std::string> loc_string_de = {
        "Applikationsart", "Bemerkungen zur Dosierung",
        
        // Sheet 1
        "max. verabreichte Tagesdosis", "Zulassungsnummer",
        
        // Sheet 2
        "max TD Tr 1", "max TD Tr 2", "max TD Tr 3",
        "Peripartale Dosierung", "Bemerkungen zur peripartalen Dosierung",
        
        // Other strings
        "Art der Anwendung", "Wirkstoff", "Hauptindikation", "Indikation",
        "stillzeit", "schwangerschaft"
    };
    std::vector<std::string> loc_string_fr = { // TODO: verify translations
        "Type d’application", "Commentaires sur dosage",
        
        // Sheet 1
        "dose quotidienne max", "Approbation",
        
        // Sheet 2
        "max TD Tr 1", "max TD Tr 2", "max TD Tr 3",
        "Peripartum posologie", "Commentaires sur périnatale posologie",
        
        // Other strings
        "Type d'utilisation", "Substance active", "Indication principale", "Indication",
        "allaitement", "grossesse"
    };
    std::vector<std::string> loc_string_en = {
        "Type of application", "Comments on dosage",
        
        // Sheet 1
        "max daily dose", "Approval",

        // Sheet 2
        "max TD Tr 1", "max TD Tr 2", "max TD Tr 3",
        "Dose adjustment", "Peripartum dosage", "Comments on peripartum dosage",
        
        // Other strings
        "Type of use", "Active Substance", "Main Indication", "Indication",
        "breastfeeding", "pregnancy"
    };
    std::map<std::string, std::string> localizedResourcesMap;

    // Used when language != "de"
    std::map<std::string, std::string> deeplTranslatedMap;

    ////////////////////////////////////////////////////////////////////////////

    // First sheet
    const std::vector<std::string> requiredColumnVec = {
        LOC_KEY_TH_TYPE, LOC_KEY_TH_MAX_DAILY
    };
    std::map<std::string, bool> optionalColumnMap = {
        {LOC_KEY_TH_COMMENT, false},
        {LOC_KEY_TH_APPROVAL, false}
    };

    // Second sheet
    const std::vector<std::string> requiredColumnVec_2 = {
        LOC_KEY_TH_TYPE
    };
    std::map<std::string, bool> optionalColumnMap_2 = {
        {LOC_KEY_TH_COMMENT, false},
        {LOC_KEY_TH_MAX1, false},
        {LOC_KEY_TH_MAX2, false},
        {LOC_KEY_TH_MAX3, false},
        {LOC_KEY_TH_PERIDOSE, false},
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

// See also src/sap/main.cpp validateAndAdd()
std::string getLocalized(const std::string &language,
                         const std::string &s)
{
    if (language == "de")
        return s;
    
    if (s.empty())
        return s;

    // No localization if it starts with a number
    if (std::isdigit(s[0]))
        return s;
    
    // No localization if it starts with a number, but after a space: " 80mg"
    if (std::isspace(s[0]) && std::isdigit(s[1]))
        return s;
    
    // Treat this as an empty cell
    if (s == "-")
        return {};

#ifdef DEBUG
    if (deeplTranslatedMap[s].empty())
        std::clog << "Empty translation for <" << s << ">" << std::endl;

    assert(!deeplTranslatedMap[s].empty());
#endif
    return deeplTranslatedMap[s];
}

// Define deeplTranslatedMap
// key "input/deepl.in.txt"
// val "input/deepl.out.fr.txt"
void getDeeplTranslationMap(const std::string &dir,
                            const std::string &job,
                            const std::string &language,
                            std::map<std::string, std::string> &map)
{
    try {
        std::string dotJob;
        if (!job.empty())
            dotJob = "." + job;
        
        std::ifstream ifsKey(dir + "/deepl" + dotJob + ".in.txt");
        std::ifstream ifsValue(dir + "/deepl" + dotJob + ".out." + language + ".txt");   // translated by deepl.sh
#ifdef WITH_DEEPL_MANUALLY_TRANSLATED
        std::ifstream ifsValue2(dir + "/deepl" + dotJob + ".out2." + language + ".txt"); // translated manually
#endif
        
        std::string key, val;
        while (std::getline(ifsKey, key)) {
            
            std::getline(ifsValue, val);
#ifdef WITH_DEEPL_MANUALLY_TRANSLATED
            if (val.empty())                    // DeepL failed to translate it
                std::getline(ifsValue2, val);   // Get it from manually translated file
#endif
#ifdef DEBUG
            assert(!val.empty());
#endif

            map.insert(std::make_pair(key, val));
        }
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
}
    
// TODO: use a set of ATCs to speed up the lookup
void parseXLXS(const std::string &inDir,
               const std::string &inFile,
               const std::string &language)
{
#ifdef DEBUG
    assert(loc_string_key.size == loc_string_de.size);
    assert(loc_string_key.size == loc_string_fr.size);
    assert(loc_string_key.size == loc_string_en.size);
#endif
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

        // Create localization map for string resources NOT from the input file
        for (int i=0; i< loc_string_key.size(); i++)
            localizedResourcesMap.insert(std::make_pair(loc_string_key[i], loc_string[i]));

        // Create localization map for string resources from the input file (translated with DeepL)
        if (language != "de")
            getDeeplTranslationMap(inDir, "sappinfo", language, deeplTranslatedMap);
    }

    const std::string &filename = inDir + inFile;
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
            for (auto cell : row) {
                xlnt::column_t::index_t col_idx = cell.column_index();
                std::clog << i++
                << "\t" << xlnt::column_t::column_string_from_index(col_idx)
                << "\t<" << cell.to_string() << ">" << std::endl;
            }
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
        bf.c.activeSubstance = getLocalized(language, aSingleRow[COLUMN_G]);
        bf.c.mainIndication = getLocalized(language, aSingleRow[COLUMN_B]);
        bf.c.indication = getLocalized(language, aSingleRow[COLUMN_C]);
        bf.c.typeOfApplication = getLocalized(language, aSingleRow[COLUMN_H]);
        bf.c.link = aSingleRow[COLUMN_S]; if (bf.c.link == "nein") bf.c.link.clear();
        bf.c.comments = getLocalized(language, aSingleRow[COLUMN_J]);
        bf.approval = aSingleRow[COLUMN_Q];
        bf.maxDailyDose = getLocalized(language, aSingleRow[COLUMN_I]);
        breastFeedVec.push_back(bf);
#ifdef DEBUG_SAPPINFO
        if (aSingleRow[COLUMN_R] == "J02AC01") {
            std::clog
            << "Art der Anwendung: " << ws.title()
            << "\n\t R ATC: <" << aSingleRow[COLUMN_R] << ">"
            << "\n\t G Wirkstoff: <" << aSingleRow[COLUMN_G] << ">"
            << "\n\t B Hauptindikation: <" << aSingleRow[COLUMN_B] << ">"
            << "\n\t C Indikation: <" << aSingleRow[COLUMN_C] << ">"
            << "\n\t H Applikationsart: <" << aSingleRow[COLUMN_H] << ">"
            << "\n\t I max: <" << aSingleRow[COLUMN_I] << ">"
            << "\n\t J comment: <" << aSingleRow[COLUMN_J] << ">"
            << "\n\t Q approval: <" << aSingleRow[COLUMN_Q] << ">"
            << "\n\t U Filter: <" << aSingleRow[COLUMN_U] << ">"
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
            for (auto cell : row) {
                xlnt::column_t::index_t col_idx = cell.column_index();
                std::clog << i++
                << "\t" << xlnt::column_t::column_string_from_index(col_idx)
                << "\t<" << cell.to_string() << ">" << std::endl;
            }
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
        pr.c.activeSubstance = getLocalized(language, aSingleRow[COLUMN_2_G]);
        pr.c.mainIndication = getLocalized(language, aSingleRow[COLUMN_2_B]);
        pr.c.indication = getLocalized(language, aSingleRow[COLUMN_2_C]);
        pr.c.typeOfApplication = getLocalized(language, aSingleRow[COLUMN_2_H]);
        pr.c.link = aSingleRow[COLUMN_2_AA]; if (pr.c.link == "nein") pr.c.link.clear();
        pr.c.comments = getLocalized(language, aSingleRow[COLUMN_2_L]);
        pr.max1 = getLocalized(language, aSingleRow[COLUMN_2_I]);
        pr.max2 = getLocalized(language, aSingleRow[COLUMN_2_J]);
        pr.max3 = getLocalized(language, aSingleRow[COLUMN_2_K]);
        pr.periDosi = aSingleRow[COLUMN_2_M];
        pr.periBeme = getLocalized(language, aSingleRow[COLUMN_2_N]);
        pregnancyVec.push_back(pr);
#ifdef DEBUG_SAPPINFO
        if (aSingleRow[COLUMN_2_Z] == "J01FA01")
        {
            std::clog
            << "Art der Anwendung: " << ws.title()
            << "\n\t Z ATC: <" << aSingleRow[COLUMN_2_Z] << ">"
            << "\n\t G Wirkstoff: <" << aSingleRow[COLUMN_2_G] << ">"
            << "\n\t B Hauptindikation: <" << aSingleRow[COLUMN_2_B] << ">"
            << "\n\t C Indikation: <" << aSingleRow[COLUMN_2_C] << ">"
            << "\n\t H Applikationsart: <" << aSingleRow[COLUMN_2_H] << ">"
            << "\n\t AA SAPP-Monographie: <" << aSingleRow[COLUMN_2_AA] << ">"
            << "\n\t I max TD Tr 1: <" << aSingleRow[COLUMN_2_I] << ">"
            << "\n\t J max TD Tr 2: <" << aSingleRow[COLUMN_2_J] << ">"
            << "\n\t K max TD Tr 3: <" << aSingleRow[COLUMN_2_K] << ">"
            << "\n\t M Peripartale Dosierung: <" << aSingleRow[COLUMN_2_M] << ">"
            << "\n\t N Bemerkungen zur peripartalen Dosierung: <" << aSingleRow[COLUMN_2_N] << ">"
            << "\n\t AC Filter: <" << aSingleRow[COLUMN_2_AC] << ">"
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
            !b.c.comments.empty())
        {
            optionalColumnMap[LOC_KEY_TH_COMMENT] = true;
            numColumns++;
        }

        if (!optionalColumnMap[LOC_KEY_TH_APPROVAL] &&
            !b.approval.empty())
        {
            optionalColumnMap[LOC_KEY_TH_APPROVAL] = true;
            numColumns++;
        }
#endif

        // Start defining the HTML code
        std::string textBeforeTable;
        {
            textBeforeTable += localizedResourcesMap[LOC_KEY_TYPE] + ": " + localizedResourcesMap[LOC_KEY_SHEET1] + "<br />\n";
            textBeforeTable += "ATC-Code: " + b.c.atcCodes + "<br />\n";
            textBeforeTable += localizedResourcesMap[LOC_KEY_ACT_SUBST] + ": " + b.c.activeSubstance + "<br />\n";
            if (!b.c.mainIndication.empty())
                textBeforeTable += localizedResourcesMap[LOC_KEY_MAIN_INDIC] + ": " + b.c.mainIndication + "<br />\n";

            if (!b.c.indication.empty())
                textBeforeTable += localizedResourcesMap[LOC_KEY_INDICATION] + ": " + b.c.indication + "<br />\n";
            
            if (!b.c.link.empty())
                textBeforeTable += "<a href=\"" + b.c.link + "\">Sappinfo Monographie</a>" + "<br />\n"; // TODO: localize
        }
        html += "\n<p class=\"spacing1\">" + textBeforeTable + "</p>\n";

        std::string tableColGroup(COL_SPAN_L + std::to_string(numColumns) + COL_SPAN_R);
        tableColGroup = "<colgroup>" + tableColGroup + "</colgroup>";
        
        std::string tableHeader;
        tableHeader.clear();

        std::string tableBody;
        tableBody.clear();
        
        {
            tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_TYPE] + TAG_TH_R;        // col H
            tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_MAX_DAILY] + TAG_TH_R;   // col I

            if (optionalColumnMap[LOC_KEY_TH_COMMENT])
                tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_COMMENT] + TAG_TH_R;  // col J
            
            if (optionalColumnMap[LOC_KEY_TH_APPROVAL])
                tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_APPROVAL] + TAG_TH_R; // col Q
            
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
                tableRow += TAG_TD_L + b.c.comments + TAG_TD_R;

            if (optionalColumnMap[LOC_KEY_TH_APPROVAL])
                tableRow += TAG_TD_L + b.approval + TAG_TD_R;

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

        if (!optionalColumnMap_2[LOC_KEY_TH_COMMENT] &&
            !p.c.comments.empty())
        {
            optionalColumnMap_2[LOC_KEY_TH_COMMENT] = true;
            numColumns++;
        }

        if (!optionalColumnMap_2[LOC_KEY_TH_PERIDOSE] &&
            !p.periDosi.empty())
        {
            optionalColumnMap_2[LOC_KEY_TH_PERIDOSE] = true;
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
            textBeforeTable += localizedResourcesMap[LOC_KEY_TYPE] + ": " + localizedResourcesMap[LOC_KEY_SHEET2] + "<br />\n";
            textBeforeTable += "ATC-Code: " + p.c.atcCodes + "<br />\n";
            textBeforeTable += localizedResourcesMap[LOC_KEY_ACT_SUBST] + ": " + p.c.activeSubstance + "<br />\n";
            if (!p.c.mainIndication.empty())
                textBeforeTable += localizedResourcesMap[LOC_KEY_MAIN_INDIC] + ": " + p.c.mainIndication + "<br />\n";
            
            if (!p.c.indication.empty())
                textBeforeTable += localizedResourcesMap[LOC_KEY_INDICATION] + ": " + p.c.indication + "<br />\n";
            
            if (!p.c.link.empty())
                textBeforeTable += "<a href=\"" + p.c.link + "\">Sappinfo Monographie</a>" + "<br />\n"; // TODO: localize
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

            if (optionalColumnMap_2[LOC_KEY_TH_COMMENT])
                tableHeader += TAG_TH_L + localizedResourcesMap[LOC_KEY_TH_COMMENT] + TAG_TH_R;

            if (optionalColumnMap_2[LOC_KEY_TH_PERIDOSE])
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

            if (optionalColumnMap_2[LOC_KEY_TH_COMMENT])
                tableRow += TAG_TD_L + p.c.comments + TAG_TD_R;

            if (optionalColumnMap_2[LOC_KEY_TH_PERIDOSE])
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
