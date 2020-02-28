//
//  peddose.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 15 Feb 2019
//

#include <iostream>
#include <set>
#include <map>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "peddose.hpp"
#include "atc.hpp"
#include "report.hpp"

#include "html_tags.h"

namespace pt = boost::property_tree;

namespace PED
{
    // Parse-phase stats
    unsigned int statsCasesCount = 0;
    unsigned int statsIndicationsCount = 0;
    unsigned int statsCodesCount = 0;
    unsigned int statsDosagesCount = 0;

    unsigned int statsCode_ALTERRELATION = 0;
    unsigned int statsCode_FG = 0;
    unsigned int statsCode_GEWICHT = 0; // Weight
    unsigned int statsCodeAtc = 0;
    unsigned int statsCodeDOSISTYP = 0;
    unsigned int statsCodeDOSISUNIT = 0;
    unsigned int statsCodeEVIDENZ = 0;
    unsigned int statsCodeRoa = 0;
    unsigned int statsCodeZEIT = 0; // Time

    // Usage stats
    unsigned int statsCasesForAtcFoundCount = 0;
    unsigned int statsCasesForAtcNotFoundCount = 0;
    unsigned int statsTablesCount = 0;

    std::vector<_case> caseVec;
    std::set<std::string> caseCaseIDSet; // TODO: obsolete
    std::set<std::string> caseAtcCodeSet;// TODO: obsolete
    std::set<std::string> caseRoaCodeSet;// TODO: obsolete

    std::map<std::string, _indication> indicationMap; // key is IndicationKey

    std::map<std::string, _code> codeAlterMap; // key is CodeValue
    std::map<std::string, _code> codeAtcMap; // key is CodeValue
    std::map<std::string, _code> codeDosisUnitMap; // key is CodeValue
    std::map<std::string, _code> codeRoaMap; // key is CodeValue
    std::map<std::string, _code> codeZeitMap; // key is CodeValue
    //std::vector<_code> codeRoaVec;
    std::set<std::string> codeRoaCodeSet;

    std::vector<_dosage> dosageVec;
    std::set<std::string> dosageCaseIDSet;// TODO: obsolete

    std::vector<std::string> blacklist;

#define TH_KEY_AGE      "age"
#define TH_KEY_WEIGHT   "weight"
#define TH_KEY_TYPE     "type"
#define TH_KEY_DOSE     "dose"
#define TH_KEY_REPEAT   "repeat"
#define TH_KEY_ROA      "roa"
#define TH_KEY_MAX      "max"
#define TH_KEY_REM      "remark"

    const std::vector<std::string> th_key = {
        TH_KEY_AGE, TH_KEY_WEIGHT, TH_KEY_TYPE, TH_KEY_DOSE,
        TH_KEY_REPEAT, TH_KEY_ROA, TH_KEY_MAX, TH_KEY_REM
    };
    std::vector<std::string> th_de = {
        "Alter", "Gewicht", "Art der Anwendung", "Dosierung",
        "Tägliche Wiederholungen", "ROA", "Max. tägliche Dosis", "Bemerkung"
    };
    std::vector<std::string> th_fr = {
        "Âge", "Poids", "Type d'utilisation", "Posologie",
        "Répétitions quotidiennes", "ROA", "Dose quotidienne maximale", "Remarque"
    };
    std::vector<std::string> th_en = {
        "Age", "Weight", "Type of use", "Dosage",
        "Daily repetitions", "ROA", "Max. daily dose", "Remark"
    };
    std::map<std::string, std::string> thTitleMap;
    std::string indicationTitle;

static std::string getAbbreviation(const std::string s)
{
    return codeDosisUnitMap[s].description;
}

static
void printFileStats(const std::string &filename)
{
    REP::html_h2("swisspeddose.ch");
    //REP::html_p(std::string(basename((char *)filename.c_str())));
    REP::html_p(filename);

    REP::html_h3("Cases " + std::to_string(statsCasesCount));
    REP::html_start_ul();
    REP::html_li("<CaseID> set: " + std::to_string(caseCaseIDSet.size()));
    REP::html_li("<ATCCode> set: " + std::to_string(caseAtcCodeSet.size()));
    REP::html_li("<ROACode> set: " + std::to_string(caseRoaCodeSet.size()));
    REP::html_end_ul();

    REP::html_h3("Indications " + std::to_string(statsIndicationsCount));
    REP::html_start_ul();
    REP::html_li("<IndicationKey> map: " + std::to_string(indicationMap.size()));
    REP::html_end_ul();

    REP::html_h3("Dosages " + std::to_string(statsDosagesCount));
    REP::html_start_ul();
    REP::html_li("<CaseId> set: " + std::to_string(dosageCaseIDSet.size()));
    REP::html_li("vec: " + std::to_string(dosageVec.size()));
    REP::html_end_ul();

    REP::html_h3("Codes " + std::to_string(statsCodesCount));
    REP::html_start_ul();
    REP::html_li("_ALTERRELATION: " + std::to_string(statsCode_ALTERRELATION));
    REP::html_li("_FG: " + std::to_string(statsCode_FG));
    REP::html_li("_GEWICHT: " + std::to_string(statsCode_GEWICHT));
    REP::html_li("ATC: " + std::to_string(statsCodeAtc)
#ifdef DEBUG
                 + ", <CodeValue> map: " + std::to_string(codeAtcMap.size())
#endif
                 );
    REP::html_li("DOSISTYP: " + std::to_string(statsCodeDOSISTYP));
    REP::html_li("DOSISUNIT: " + std::to_string(statsCodeDOSISUNIT)
#ifdef DEBUG
                 + ", <CodeValue> map: " + std::to_string(codeDosisUnitMap.size())
#endif
                 );
    REP::html_li("EVIDENZ: " + std::to_string(statsCodeEVIDENZ));
    REP::html_li("ROA: " + std::to_string(statsCodeRoa)
#ifdef DEBUG
                 + ", <CodeValue> set: " + std::to_string(codeRoaCodeSet.size())
                 + ", <CodeValue> map: " + std::to_string(codeRoaMap.size())
#endif
                 );
    REP::html_li("ZEIT: " + std::to_string(statsCodeZEIT));
    REP::html_end_ul();
}

void printUsageStats()
{
    REP::html_h2("swisspeddose.ch");

    REP::html_start_ul();
    REP::html_li("ATC with <Case>: " + std::to_string(statsCasesForAtcFoundCount));
    REP::html_li("ATC without <Case>: " + std::to_string(statsCasesForAtcNotFoundCount));
    REP::html_li("tables created: " + std::to_string(statsTablesCount));
    REP::html_end_ul();
}

void parseBlacklistTXT(const std::string &filename)
{
    try {
        std::clog << std::endl << "Reading " << filename << std::endl;
        std::ifstream file(filename);

        std::string str;
        while (std::getline(file, str))
        {
            if (str.size() == 0) {
                continue;
            }
            if (str[0] == '#') {
                continue;
            }
            blacklist.push_back(str);
        } // while

        file.close();
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
}

void parseXML(const std::string &filename,
              const std::string &language)
{
    {
        // Define localized lookup table for pedDose table header
        std::vector<std::string> &th = th_en;
        indicationTitle = "Indication";
        if (language == "de") {
            th = th_de;
            indicationTitle = "Indikation";
        }
        else if (language == "fr") {
            th = th_fr;
            indicationTitle = "Indication";
        }

        for (int i=0; i< th_key.size(); i++)
            thTitleMap.insert(std::make_pair(th_key[i], th[i]));
    }

    pt::ptree tree;

    try {
        std::clog << std::endl << "Reading " << filename << std::endl;
        pt::read_xml(filename, tree);
    }
    catch (std::exception &e) {
        std::cerr << "Line: " << __LINE__ << " Error " << e.what() << std::endl;
    }

    std::clog << "Analyzing Ped " << language << std::endl;
    int i=0;

    try {
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SwissPedDosePublication")) {
            if (v.first == "Cases") {
                statsCasesCount = v.second.size();
            }
            else if (v.first == "Indications") {
                statsIndicationsCount = v.second.size();
            }
            else if (v.first == "Codes") {
                statsCodesCount = v.second.size();
            }
            else if (v.first == "Dosages") {
                statsDosagesCount = v.second.size();
            }
        }  // FOREACH

        i = 0;
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SwissPedDosePublication.Cases")) {
            if (v.first == "Case") {
#if 0

                std::clog
                << basename((char *)__FILE__) << ":" << __LINE__
                << ", i: " << i++
                << ", # children: " << v.second.size()
                //<< ", key <" << v.first.data() << ">"       // Case
                << ", CaseTitle <" << v.second.get("CaseTitle", "") << ">"
                << std::endl;
#endif
                caseCaseIDSet.insert(v.second.get("CaseID", ""));
                caseAtcCodeSet.insert(v.second.get("ATCCode", ""));
                caseRoaCodeSet.insert(v.second.get("ROACode", ""));

                _case ca;
                ca.caseId = v.second.get("CaseID", "");
                ca.atcCode = v.second.get("ATCCode", "");
                ca.indicationKey = v.second.get("IndicationKey", "");
                ca.RoaCode = v.second.get("ROACode", "");
                caseVec.push_back(ca);
            }
        } // FOREACH Cases

        i = 0;
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SwissPedDosePublication.Indications")) {
            if (v.first == "Indication") {

                _indication in;
                if (language == "de")
                    in.name = v.second.get("IndicationNameD", "");
                else if (language == "fr")
                    in.name = v.second.get("IndicationNameF", "");
                else
                    in.name = v.second.get("IndikationNameE", ""); // English has a K

                in.recStatus = v.second.get("RecStatus", "");
                indicationMap.insert(std::make_pair(v.second.get("IndicationKey", ""), in));
            }
        }

        i = 0;
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SwissPedDosePublication.Codes")) {
            if (v.first == "Code") {
                std::string codeType = v.second.get("CodeType", "");

                _code co;
                co.value = v.second.get("CodeValue", "");
                if (language == "de")
                    co.description = v.second.get("DescriptionD", "");
                else if (language == "fr")
                    co.description = v.second.get("DecsriptionF", "");  // Note: spelling mistake
                else //if (language == "en")
                    co.description = v.second.get("DecriptionE", "");
                co.recStatus = v.second.get("RecStatus", "");

                if (codeType == "_ALTERRELATION") {
                    statsCode_ALTERRELATION++;
                    codeAlterMap.insert(std::make_pair(co.value, co));
                }
                else if (codeType == "_GEWICHT") { // Weight
                    statsCode_GEWICHT++;
                }
                else if (codeType == "_FG") {
                    statsCode_FG++;
                }
                else if (codeType == "ATC") {
                    statsCodeAtc++;
                    codeAtcMap.insert(std::make_pair(co.value, co));
                }
                else if (codeType == "DOSISTYP") {
                    statsCodeDOSISTYP++;
                }
                else if (codeType == "DOSISUNIT") {
                    statsCodeDOSISUNIT++;
                    codeDosisUnitMap.insert(std::make_pair(co.value, co));
                }
                else if (codeType == "EVIDENZ") {
                    statsCodeEVIDENZ++;
                }
                else if (codeType == "ROA") {
                    statsCodeRoa++;

                    // Both the following are still unused
                    // One of them will be used to fetch the localized description if required
                    // So far we are just using the ROA from the cases struct instead.
                    codeRoaCodeSet.insert(co.value);
                    //codeRoaVec.push_back(co);
                    codeRoaMap.insert(std::make_pair(co.value, co));
                }
                else if (codeType == "ZEIT") {  // Time
                    statsCodeZEIT++;
                    codeZeitMap.insert(std::make_pair(co.value, co));
                }
            }
        } // FOREACH Codes

        i = 0;
        BOOST_FOREACH(pt::ptree::value_type &v, tree.get_child("SwissPedDosePublication.Dosages")) {
            if (v.first == "Dosage") {
                dosageCaseIDSet.insert(v.second.get("CaseID", ""));

                _dosage dos;
                dos.key = v.second.get("DosageKey", "");

                dos.ageFrom = v.second.get("AgeFrom", "");
                dos.ageFromUnit = v.second.get("AgeFromUnit", "");
                dos.ageTo = v.second.get("AgeTo", "");
                dos.ageToUnit = v.second.get("AgeToUnit", "");

                dos.ageWeightRelation = v.second.get("AgeWeightRelation", "");
                dos.weightFrom = v.second.get("WeightFrom", "");
                dos.weightTo = v.second.get("WeightTo", "");

                dos.doseLow = v.second.get("LowerDoseRange", "");
                dos.doseHigh = v.second.get("UpperDoseRange", "");
                dos.doseUnit = v.second.get("DoseRangeUnit", "");
                dos.doseUnitRef1 = v.second.get("DoseRangeReferenceUnit1", "");
                dos.doseUnitRef2 = v.second.get("DoseRangeReferenceUnit2", "");

                dos.dailyRepetitionsLow = v.second.get("LowerRangeDailyRepetitions", "");
                dos.dailyRepetitionsHigh = v.second.get("UpperRangeDailyRepetitions", "");

                dos.maxSingleDose = v.second.get("MaxSingleDose", "");
                dos.maxSingleDoseUnit = v.second.get("MaxSingleDoseUnit", "");
                dos.maxSingleDoseUnitRef1 = v.second.get("MaxSingleDoseReferenceUnit1", "");
                dos.maxSingleDoseUnitRef2 = v.second.get("MaxSingleDoseReferenceUnit2", "");

                dos.maxDailyDose = v.second.get("MaxDailyDose", "");
                dos.maxDailyDoseUnit = v.second.get("MaxDailyDoseUnit", "");
                dos.maxDailyDoseUnitRef1 = v.second.get("MaxDailyDoseReferenceUnit1", "");
                dos.maxDailyDoseUnitRef2 = v.second.get("MaxDailyDoseReferenceUnit2", "");

                if (language == "de")
                    dos.remarks = v.second.get("RemarksD", "");
                else if (language == "fr")
                    dos.remarks = v.second.get("RemarksF", "");
                else if (language == "it")
                    dos.remarks = v.second.get("RemarksI", "");
                else //if (language == "en")
                    dos.remarks = v.second.get("RemarksE", "");

                dos.roaCode = v.second.get("ROACode", "");
                dos.caseId = v.second.get("CaseID", "");
                dos.type = v.second.get("TypeOfCase", "");

                dosageVec.push_back(dos);
            }
        } // FOREACH Dosages
    } // try
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", Error " << e.what()
        << std::endl;
    }

    printFileStats(filename);
}

std::string getDescriptionByAtc(const std::string &atc)
{
    return codeAtcMap[atc].description;
}

// The input string is in the format "atccode[,atccode]*"
std::string getTextByAtcs(const std::string atcs)
{
    std::string text;
    std::string firstAtc = ATC::getFirstAtc(atcs);

    return codeAtcMap[firstAtc].description;
}

// There could be multiple cases for the same ATC. Return a vector
void getCasesByAtc(const std::string &atc, std::vector<_case> &cases)
{
    for (auto c : caseVec) {
        if (c.atcCode == atc)
            cases.push_back(c);
    }
}

std::string getIndicationByKey(const std::string &key)
{
    return indicationMap[key].name;
}

//_dosage getDosageById(const std::string &id)
void getDosageById(const std::string &id, std::vector<_dosage> &dosages)
{
    for (auto d : dosageVec)
        if (d.caseId == id)
            dosages.push_back(d);
}

// Each "case" generates one table
// One ATC can have mnay cases and therefore multiple tables
//
std::string getHtmlByAtc(const std::string atc)
{
    std::string html;
    std::vector<_case> cases;
    PED::getCasesByAtc(atc, cases);

    if (cases.empty()) {
        statsCasesForAtcNotFoundCount++;
        return {};  // empty string
    }

    statsCasesForAtcFoundCount++;

    html.clear();

    for (auto ca : cases) {
        auto description = PED::getDescriptionByAtc(atc);
        auto indication = PED::getIndicationByKey(ca.indicationKey);
        std::vector<_dosage> dosages;
        PED::getDosageById(ca.caseId, dosages);

        // Check for optional columns
        std::map<std::string, bool> optionalColumnMap = {
            {TH_KEY_ROA, false},
            {TH_KEY_WEIGHT, false},
            {TH_KEY_TYPE, false},
            {TH_KEY_MAX, false},
            {TH_KEY_REPEAT, false},
            {TH_KEY_REM, false}
        };
        int numColumns = th_key.size() - optionalColumnMap.size();
        for (auto dosage : dosages) {
            if (!optionalColumnMap[TH_KEY_ROA] &&
                (dosage.roaCode != ca.RoaCode))
            {
                optionalColumnMap[TH_KEY_ROA] = true;
                numColumns++;
            }

            if (!optionalColumnMap[TH_KEY_TYPE] &&
                (dosage.type != dosages[0].type))
            {
                optionalColumnMap[TH_KEY_TYPE] = true;
                numColumns++;
            }

            if (!optionalColumnMap[TH_KEY_REM] &&
                !dosage.remarks.empty())
            {
                optionalColumnMap[TH_KEY_REM] = true;
                numColumns++;
            }

            // Check if all weights are 0 to also skip weight column
            if (!optionalColumnMap[TH_KEY_WEIGHT] &&
                ((dosage.weightFrom != "0") ||
                (dosage.weightTo != "0")))
            {
                optionalColumnMap[TH_KEY_WEIGHT] = true;
                numColumns++;
            }

            if (!optionalColumnMap[TH_KEY_MAX] &&
                (dosage.maxDailyDose != "0"))
            {
                optionalColumnMap[TH_KEY_MAX] = true;
                numColumns++;
            }

            if (!optionalColumnMap[TH_KEY_REPEAT] &&
                ((dosage.dailyRepetitionsLow != "0") ||
                 (dosage.dailyRepetitionsHigh != "0")))
            {
                optionalColumnMap[TH_KEY_REPEAT] = true;
                numColumns++;
            }
        } // for dosages

        // Start defining the HTML code
        std::string textBeforeTable;
        {
            textBeforeTable = description + " (" + ca.RoaCode + ") " + codeRoaMap[ca.RoaCode].description + "<br />\n";
            textBeforeTable += "ATC-Code: " + atc + "<br />\n";
            textBeforeTable += indicationTitle + ": " + indication;

            if (!optionalColumnMap[TH_KEY_TYPE] && !dosages[0].type.empty())
                textBeforeTable += "<br />\n" + thTitleMap[TH_KEY_TYPE] + ": " + dosages[0].type;
        }
        html += "\n<p class=\"spacing1\">" + textBeforeTable + "</p>\n";

        std::string tableColGroup(COL_SPAN_L + std::to_string(numColumns) + COL_SPAN_R);
        tableColGroup = "<colgroup>" + tableColGroup + "</colgroup>";

        std::string tableHeader;
        tableHeader.clear();

        std::string tableBody;
        tableBody.clear();

        if (dosages.size() > 0) {
            tableHeader += TAG_TH_L + thTitleMap[TH_KEY_AGE] + TAG_TH_R;

            if (optionalColumnMap[TH_KEY_WEIGHT])
                tableHeader += TAG_TH_L + thTitleMap[TH_KEY_WEIGHT] + TAG_TH_R;

            if (optionalColumnMap[TH_KEY_TYPE])
                tableHeader += TAG_TH_L + thTitleMap[TH_KEY_TYPE] + TAG_TH_R;

            tableHeader += TAG_TH_L + thTitleMap[TH_KEY_DOSE] + TAG_TH_R;

            if (optionalColumnMap[TH_KEY_REPEAT])
                tableHeader += TAG_TH_L + thTitleMap[TH_KEY_REPEAT] + TAG_TH_R;

            if (optionalColumnMap[TH_KEY_ROA])
                tableHeader += TAG_TH_L + thTitleMap[TH_KEY_ROA] + TAG_TH_R;

            if (optionalColumnMap[TH_KEY_MAX])
                tableHeader += TAG_TH_L + thTitleMap[TH_KEY_MAX] + TAG_TH_R;

            if (optionalColumnMap[TH_KEY_REM])
                tableHeader += TAG_TH_L + thTitleMap[TH_KEY_REM] + TAG_TH_R;

            tableHeader += "\n"; // for readability

            tableHeader = "<tr>" + tableHeader + "</tr>";
#ifdef WITH_SEPARATE_TABLE_HEADER
            tableHeader = "<thead>" + tableHeader + "</thead>";
#else
            tableBody += tableHeader;
#endif
        } // if dosages.size()

        for (auto dosage : dosages) {
            std::string tableRow;
            tableRow += TAG_TD_L;
            tableRow += dosage.ageFrom;
            tableRow += " " + codeZeitMap[dosage.ageFromUnit].description;
            tableRow += " - " + dosage.ageTo;
            tableRow += " " + codeZeitMap[dosage.ageToUnit].description;
            if (!dosage.ageWeightRelation.empty())
                tableRow += " " + codeAlterMap[dosage.ageWeightRelation].description;
            tableRow += TAG_TD_R;

            if (optionalColumnMap[TH_KEY_WEIGHT]) {
                tableRow += TAG_TD_L;
                tableRow += dosage.weightFrom;
                if (dosage.weightFrom != dosage.weightTo)
                    tableRow += " - " + dosage.weightTo;
                tableRow += " kg";
                tableRow += TAG_TD_R;
            }

            if (optionalColumnMap[TH_KEY_TYPE]) {
                tableRow += TAG_TD_L;
                tableRow += dosage.type;
                tableRow += TAG_TD_R;
            }

            tableRow += TAG_TD_L;
            tableRow += dosage.doseLow;
            if (dosage.doseLow != dosage.doseHigh)
                tableRow += " - " + dosage.doseHigh;
            tableRow += " " + getAbbreviation(dosage.doseUnit);
            if (!dosage.doseUnitRef1.empty())
                tableRow += "/" + getAbbreviation(dosage.doseUnitRef1);
            if (!dosage.doseUnitRef2.empty())
                tableRow += "/" + getAbbreviation(dosage.doseUnitRef2);
            tableRow += TAG_TD_R;

            if (optionalColumnMap[TH_KEY_REPEAT]) {
                tableRow += TAG_TD_L;
                tableRow += dosage.dailyRepetitionsLow;
                if (dosage.dailyRepetitionsLow != dosage.dailyRepetitionsHigh)
                    tableRow += " - " + dosage.dailyRepetitionsHigh;
                tableRow += TAG_TD_R;
            }

            if (optionalColumnMap[TH_KEY_ROA]) {
                tableRow += TAG_TD_L;
                tableRow += dosage.roaCode;
                tableRow += TAG_TD_R;
            }

            if (optionalColumnMap[TH_KEY_MAX]) {
                tableRow += TAG_TD_L;
                tableRow += dosage.maxDailyDose + " " + getAbbreviation(dosage.maxDailyDoseUnit);
                if (!dosage.maxDailyDoseUnitRef1.empty())
                    tableRow += "/" + getAbbreviation(dosage.maxDailyDoseUnitRef1);
                if (!dosage.maxDailyDoseUnitRef2.empty())
                    tableRow += "/" + getAbbreviation(dosage.maxDailyDoseUnitRef2);
                tableRow += TAG_TD_R;
            }

            if (optionalColumnMap[TH_KEY_REM]) {
                tableRow += TAG_TD_L;
                tableRow += dosage.remarks;
                tableRow += TAG_TD_R;
            }

            tableRow += "\n";  // for readability

            tableRow = "<tr>" + tableRow + "</tr>";
            tableBody += tableRow;
        } // for dosages

        tableBody = "<tbody>" + tableBody + "</tbody>";

        std::string table = tableColGroup;
#ifdef WITH_SEPARATE_TABLE_HEADER
        table += tableHeader + tableBody;
#else
        table += tableBody;
#endif
        table = TAG_TABLE_L + table + TAG_TABLE_R;
        html += table;
        statsTablesCount++;
    } // for cases

    return html;
}

void showPedDoseByAtc(const std::string atc)
{
    std::vector<_case> cases;
    PED::getCasesByAtc(atc, cases);

    if (cases.empty()) {
        std::cout << "No cases for ATC: " << atc << std::endl;
        return;
    }

    std::cout << "Ped Dose, ATC: " << atc << std::endl;

    for (auto ca : cases) {
        auto description = PED::getDescriptionByAtc(atc);
        auto indication = PED::getIndicationByKey(ca.indicationKey);

        std::vector<_dosage> dosages;
        PED::getDosageById(ca.caseId, dosages);

        std::cout
        << "\t caseId: " << ca.caseId
        << "\n\t\t " << description << " (" << ca.RoaCode << ")"
        << "\n\t\t indication: " << indication
        << std::endl;

        for (auto dosage : dosages) {
            std::cout
            << "\t\t dosage recommendation: " << dosage.key
            << "\n\t\t\t age: " << dosage.ageFrom << " " << dosage.ageFromUnit
            << ", to: " << dosage.ageTo << " " << dosage.ageToUnit

            << "\n\t\t\t dosage: " << dosage.doseLow << " - " << dosage.doseHigh << " " << dosage.doseUnit << "/" << dosage.doseUnitRef1 << "/" << dosage.doseUnitRef2

            << "\n\t\t\t daily repetitions: " << dosage.dailyRepetitionsLow << " - " << dosage.dailyRepetitionsHigh

            << "\n\t\t\t max single dose: " << dosage.maxSingleDose << " " << dosage.maxSingleDoseUnit << "/" << dosage.maxSingleDoseUnitRef1 << "/" << dosage.maxSingleDoseUnitRef2

            << "\n\t\t\t max daily dose: " << dosage.maxDailyDose << " " << dosage.maxDailyDoseUnit << "/" << dosage.maxDailyDoseUnitRef1 << "/" << dosage.maxDailyDoseUnitRef2;

            if (!dosage.remarks.empty())
                std::cout << "\n\t\t\t remarks: " << dosage.remarks;

            std::cout << std::endl;
        }
    }
}

}
