//
//  swissmedic1.cpp
//  pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 22 Jan 2019
//

#include <iostream>
#include <fstream>
#include <libgen.h>     // for basename()
#include <regex>
#include <iomanip>
#include <sstream>
#include <boost/algorithm/string.hpp>

#include <xlnt/xlnt.hpp>

#include "swissmedic1.hpp"
#include "swissmedic2.hpp"
#include "gtin.hpp"
#include "bag.hpp"
//#include "beautify.hpp"
#include "report.hpp"
#include "refdata.hpp"
#include "ddd.hpp"

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
#define COLUMN_S       18   // application field
#define COLUMN_W       22   // preparation contains narcotics
#define COLUMN_X       23   // narcotic flag

#define FIRST_DATA_ROW_INDEX    6

#define OUTPUT_FILE_SEPARATOR   ";"
#define CELL_ESCAPE             "\""

//#define DEBUG_DOSAGE_REGEX

#ifdef DEBUG_DOSAGE_REGEX
#include <set>
// To verify regular expression, see issue #72
const std::set<long int> atcTestSet = {
    7680121750195, // APHENYLBARBIT
    7680202725067, // RAPIDOCAIN
    7680207130118, // KYTTA
    7680215050132, // SUCCINOLIN
    7680216720010, // VITARUBIN
    7680221140261, // SYNTOCINON
    7680229580540, // SOLCOSERYL
    7680229580021, // Solcoseryl
    7680240350184, // AKINETON
    7680261770305, // KENACORT
    7680263960346, // SOLCOSERYL
    7680278370260, // VENORUTON
    7680290800622, // AEQUIFUSINE
    7680295508851, // GLUC
    7680295520891, // KCL
    7680295540998, // NACL
    7680613930074, // CO-VALSARTAN
    7680620790128, // Pramipexol
    7680620800025, // XEOMIN
    7680620830039, // Methrexx
    7680620830084, // METHREXX
    7680621090012, // LOSARTAN
    7680621460037, // REFACTO
    7680621830021, // DEXDOR
    7680358561021, // BLEOMYCIN
    7680359600231, // QUILONORM
    7680363520358, // NASIVIN
    7680364640017, // EFUDIX
    7680370570018, // OSPEN
    7680378870561, // BACTRIM
    7680661420022, // PERINDOPRIL
    7680662190047, // LONSURF
    7680662370029, // DESOGYNELLE
    7680662620070, // CLARISCAN
    7680651310029, // ALENDRON
    7680005920010, // TWINRIX
    7680659680056, // LYXUMIA
    7680608720017, // Calvive
    7680612150121  // CANSARTAN
};
#endif

namespace SWISSMEDIC1
{
    std::vector< std::vector<std::string> > theWholeSpreadSheet;
    std::vector<std::string> regnrs;        // padded to 5 characters (digits)
#if 1
    std::vector<pharmaRow> pharmaVec;
#else
    std::vector<std::string> packingCode;   // padded to 3 characters (digits)
    std::vector<std::string> gtin;
#endif
    std::string fromSwissmedic("ev.nn.i.H.");
    
//    // TODO: change it to a map for better performance
//    std::vector<std::string> categoryVec;
//
//    // TODO: change them to a map for better performance
//    std::vector<dosageUnits> duVec;

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
    
    auto date_format = wb.create_format().number_format(xlnt::number_format{"dd.mm.yyyy"}, xlnt::optional<bool>(true));

    std::clog << std::endl << "Reading swissmedic XLSX" << std::endl;

    unsigned int skipHeaderCount = 0;
    for (auto row : ws.rows(false)) {
        ++skipHeaderCount;
        if (skipHeaderCount <= FIRST_DATA_ROW_INDEX) {
#if 0 //def DEBUG_PHARMA
            if (skipHeaderCount == FIRST_DATA_ROW_INDEX) {
                int i=0;
                for (auto cell : row) {
                    xlnt::column_t::index_t col_idx = cell.column_index();
                    std::clog << i++
                    << "\t" << xlnt::column_t::column_string_from_index(col_idx)
                    << "\t<" << cell.to_string() << ">" << std::endl;
                }
            }
#endif
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
        regnrs.push_back(pr.rn5);

        pr.dosageNr = aSingleRow[COLUMN_B];
        pr.code3 = GTIN::padToLength(3, aSingleRow[COLUMN_K]);
        
        // Precalculate gtin
        std::string gtin12 = "7680" + pr.rn5 + pr.code3;
        char checksum = GTIN::getGtin13Checksum(gtin12);
        pr.gtin13 = gtin12 + checksum;

#if 0  // Old way
        pr.name = aSingleRow[COLUMN_C];
        pr.galenicForm = aSingleRow[COLUMN_M];
#else
        std::string nameString = aSingleRow[COLUMN_C];
        boost::replace_all(nameString, CELL_ESCAPE, ""); // double quotes interfere with CSV
        boost::replace_all(nameString, ";", ","); // sometimes the galenic form is after ';' not ','

        std::vector<std::string> nameComponents;
        boost::algorithm::split(nameComponents, nameString, boost::is_any_of(","));

        // Use the name only up to first comma (Issue #68, 5)
        pr.name = nameComponents[0];

        // Use words after last comma (Issue #68, 6)
        pr.galenicForm = nameComponents[nameComponents.size()-1];
        boost::algorithm::trim(pr.galenicForm);
#endif

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

        pr.categoryPack = aSingleRow[COLUMN_N];
        if ((pr.categoryPack == "A") && (aSingleRow[COLUMN_W] == "a"))
            pr.categoryPack += "+";

        pharmaVec.push_back(pr);
    }

    printFileStats(filename);
}

//// Return count added
//int getAdditionalNames(const std::string &rn,
//                       std::set<std::string> &gtinUsedSet,
//                       GTIN::oneFachinfoPackages &packages,
//                       const std::string &language)
//{
//    std::set<std::string>::iterator it;
//    int countAdded = 0;
//
//    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
//        std::string rn5 = regnrs[rowInt];
//        if (rn5 != rn)
//            continue;
//
//        std::string g13 = gtin[rowInt];
//        it = gtinUsedSet.find(g13);
//        if (it == gtinUsedSet.end()) { // not found list of used GTINs, we must add the name
//            countAdded++;
//            statsAugmentedGtinCount++;
//            statsTotalGtinCount++;
//
//            std::string onePackageInfo;
//#ifdef DEBUG_IDENTIFY_NAMES
//            onePackageInfo += "swm+";
//#endif
//            onePackageInfo += theWholeSpreadSheet.at(rowInt).at(COLUMN_C);
//            BEAUTY::beautifyName(onePackageInfo);
//            // Verify presence of dosage
//            std::regex r(R"(\d+)");
//            if (!std::regex_search(onePackageInfo, r)) {
//                statsRecoveredDosage++;
//                //std::clog << "no dosage for " << name << std::endl;
//                onePackageInfo += " " + duVec[rowInt].dosage;
//                onePackageInfo += " " + duVec[rowInt].units;
//            }
//
//            // See RealExpertInfo.java:1544
//            //  "a.H." --> "ev.nn.i.H."
//            //  "p.c." --> "ev.ep.e.c."
//            if (language == "fr")
//                fromSwissmedic = "ev.ep.e.c.";
//
//            std::string paf = BAG::getPricesAndFlags(g13, fromSwissmedic, categoryVec[rowInt]);
//            if (!paf.empty())
//                onePackageInfo += paf;
//
//            gtinUsedSet.insert(g13);
//            packages.gtin.push_back(g13);
//            packages.name.push_back(onePackageInfo);
//        }
//    }
//
//    if (countAdded > 0)
//        statsAugmentedRegnCount++;
//
//    return countAdded;
//}

//int countRowsWithRn(const std::string &rn)
//{
//    int count = 0;
//    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
//        std::string gtin_5 = regnrs[rowInt];
//        // TODO: to speed up do a numerical comparison so that we can return when gtin5>rn
//        // assuming that column A is sorted
//        if (gtin_5 == rn)
//            count++;
//    }
//
//    return count;
//}
    
//bool findGtin(const std::string &gtin)
//{
//    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
//        std::string rn5 = regnrs[rowInt];
//        std::string code3 = packingCode[rowInt];
//        std::string gtin12 = "7680" + rn5 + code3;
//
//#if 0
//        // We could also recalculate and verify the checksum
//        // but such verification has already been done when parsing the files
//        char checksum = GTIN::getGtin13Checksum(gtin12);
//
//        if (checksum != gtin[12]) {
//            std::cerr
//            << basename((char *)__FILE__) << ":" << __LINE__
//            << ", GTIN error, expected:" << checksum
//            << ", received" << gtin[12]
//            << std::endl;
//        }
//#endif
//
//        // The comparison is only the first 12 digits, without checksum
//        if (gtin12 == gtin.substr(0,12)) // pos, len
//            return true;
//    }
//
//    return false;
//}

//std::string getApplication(const std::string &rn)
//{
//    std::string app;
//
//    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
//        if (rn == regnrs[rowInt]) {
//            app = theWholeSpreadSheet.at(rowInt).at(COLUMN_S) + " (Swissmedic)";
//            break;
//        }
//    }
//
//    return app;
//}

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
    
// Issue #68, 9
void getPackageSizeNumericalFromName(std::string &calculatedDosage)
{
    // If it contains + or - use as is
    if (calculatedDosage.find_first_of("+-") != std::string::npos)
        return;

    std::string stringMassaged = boost::to_lower_copy<std::string>(calculatedDosage); // "X" --> "x"
    boost::replace_all(stringMassaged, ",", "."); // "0,5" --> "0.5"
    
    if (stringMassaged.find("x") == std::string::npos) {      // It doesn't contain "x"
        if (stringMassaged.find("ml") != std::string::npos)   // but it contains "ml"
            calculatedDosage = "1";
        
        return;
    }

    std::regex rgx(R"((\d+((\.|,)\d+)?)\s*x\s*(\d+((\.|,)\d+)?))");  // tested at https://regex101.com
    std::smatch match;
    if (std::regex_search(stringMassaged, match, rgx)) {
#if 0 //def DEBUG
        std::clog
        << "calculatedDosage <" << calculatedDosage << ">"
        << ", match.size <" << match.size() << ">"
        << std::endl;
        for (auto m : match) {
            std::clog
            << "\t <" << m << ">"
            << std::endl;
        }
#endif
        std::string as = match[1];
        std::string bs = match[4];
        double a = std::atof(as.c_str());
        double b = std::atof(bs.c_str());
#if 0 //def DEBUG
        std::clog
        << "calculatedDosage <" << calculatedDosage << ">"
        << ", a " << a
        << ", b " << b
        << ", axb " << (a*b)
        << std::endl;
#endif
        calculatedDosage = std::to_string(a*b);
    }
}
    
// Issue #72 Extract "Dosierung" from "Präparat" with an almighty regular expression
std::string getDosageFromName(const std::string &name)
{
    // TODO: use a separate regex if the name ends with "stk"

    std::string dosage;
    std::regex rgx(R"(\d+(\.\d+)?\s*(mg|g(\s|$)|i.u.|e(\s|$)|mcg|ie|mmol)(\s?\/\s?(\d(\.\d+)?)*\s*(ml|g|mcg))*)");  // tested at https://regex101.com
    std::smatch match;
    if (std::regex_search(name, match, rgx))
        dosage = match[0];
    
    // TODO: trim trailing space
    // TODO: change " / " to "/"
    
    return dosage;
}

//dosageUnits getByGtin(const std::string &g)
//{
//    dosageUnits du;
//
//    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++)
//        if (gtin[rowInt] == g) {
//            du = duVec[rowInt];
//            break;
//        }
//
//    return du;
//}

void createCSV(const std::string &outDir)
{
    std::ofstream ofs;
    std::string filename = outDir + "/pharma.csv";
    ofs.open(filename);
 
    std::clog << std::endl << "Creating CSV" << std::endl;

    ofs
    << "registration_nbr" << OUTPUT_FILE_SEPARATOR      // A
    << "package_nbr" << OUTPUT_FILE_SEPARATOR           // B
    << "swissmedic_nbr" << OUTPUT_FILE_SEPARATOR        // C
    << "gtin" << OUTPUT_FILE_SEPARATOR                  // D
    << "product_name" << OUTPUT_FILE_SEPARATOR          // E
    << "galenic_form" << OUTPUT_FILE_SEPARATOR          // F
    << "dosis" << OUTPUT_FILE_SEPARATOR                 // G
    << "package_content" << OUTPUT_FILE_SEPARATOR       // H
    << "content_numeric" << OUTPUT_FILE_SEPARATOR       // I
    << "price_exfactory" << OUTPUT_FILE_SEPARATOR       // J
    << "price_public" << OUTPUT_FILE_SEPARATOR          // K
    << "admission_holder" << OUTPUT_FILE_SEPARATOR      // L
    << "swissmedic_cat" << OUTPUT_FILE_SEPARATOR        // M
    << "sl_list" << OUTPUT_FILE_SEPARATOR               // N
    << "sl_date" << OUTPUT_FILE_SEPARATOR               // O
    << "registration_date" << OUTPUT_FILE_SEPARATOR     // P
    << "expiration_date" << OUTPUT_FILE_SEPARATOR       // Q
    << "export_product" << OUTPUT_FILE_SEPARATOR        // R
    << "generic" << OUTPUT_FILE_SEPARATOR               // S
    << "index_therapeuticus_bag" << OUTPUT_FILE_SEPARATOR // T
    << "index_therapeuticus_swissmedic" << OUTPUT_FILE_SEPARATOR // U
    << "narcotic" << OUTPUT_FILE_SEPARATOR              // V
    << "vaccine" << OUTPUT_FILE_SEPARATOR               // W
    << "ddd"  << OUTPUT_FILE_SEPARATOR                  // X
    << "ddd_calculation"                                // Y
    << std::endl;
    
    for (auto pv : pharmaVec) {

        std::string cat = getCategoryPackByGtin(pv.gtin13);
        std::string paf = BAG::getPricesAndFlags(pv.gtin13, "", cat);
        BAG::packageFields fromBag = BAG::getPackageFieldsByGtin(pv.gtin13);
        std::string auth = SWISSMEDIC2::getAuthorizationByAtc(pv.rn5, pv.dosageNr);
        
        // Column E
        // Take the name first from Refdata based on GTIN
        std::string name = REFDATA::getNameByGtin(pv.gtin13);
        if (name.empty())
            name = pv.name;

        // Column G
        std::string dosage = getDosageFromName(name);
#ifdef DEBUG_DOSAGE_REGEX
        static int k=1;
        if (atcTestSet.find(std::stol(pv.gtin13)) != atcTestSet.end()) {
            std::clog << k++ << "."
            << "\t <" << name << ">"
            << "\t\t\t <" << dosage << ">"
            << std::endl;
        }
#endif
        
        // Column I
        std::string calculatedDosageString = pv.du.dosage;
        getPackageSizeNumericalFromName(calculatedDosageString);

        // Columns N, S
        std::string bagFlagSL;
        std::string bagFlagGeneric;
        for (auto s : fromBag.flags) {
            if (s == "SL")
                bagFlagSL = s;

            if ((s == "G") || (s == "O"))
                bagFlagGeneric = s;
        }
        
        // Column T
        std::string itCodeBag = BAG::getLongestItCodeByGtin(pv.gtin13);
        
        // Column X
        std::string dailyCostString;
        std::string dailyCostFormula;
        std::string atc = getAtcFromFirstRn(pv.rn5);
        if (!atc.empty()) {
            double daily_dosage_mg;
            if (DDD::getDailyDosage_mg_byATC(atc, &daily_dosage_mg)) {
                if (!dosage.empty()) {
                    // Split column G dosage in value and units, convert to package_mg
                    std::string dosageValueString;
                    std::string dosageUnitsString;
                    std::regex rgx(R"((\d+(.\d+)?)\s(mg|g))");  // tested at https://regex101.com
                    std::smatch match;
                    if (std::regex_search(name, match, rgx)) {
#ifdef DEBUG
                        std::clog
                        << "dosage <" << dosage << ">"
                        << ", match.size <" << match.size() << ">"
                        << std::endl;
                        for (auto m : match) {
                            std::clog
                            << "\t <" << m << ">"
                            << std::endl;
                        }
#endif
                        if (match.size() == 4) {
                            dosageValueString = match[1];
                            dosageUnitsString = match[3];
                            double package_mg = std::atof(dosageValueString.c_str());
                            if (dosageUnitsString == "g")
                                package_mg *= 1000;

                            // Multiply by column K --> package_cost
                            if (!fromBag.pp.empty() && !calculatedDosageString.empty()) {
                                
                                // Multiply bu column I
                                double calculatedDosageValue = std::atof(calculatedDosageString.c_str());
                                package_mg *= calculatedDosageValue;

                                double package_cost = std::atof(fromBag.pp.c_str());
                                
                                // Divide package_cost by package_mg --> package_cost_per_mg
                                double package_cost_per_mg = package_cost / package_mg;

                                // Multiply by DDD daily_dosage_mg --> daily_cost
                                double dailyCost = package_cost_per_mg * daily_dosage_mg;
                            
                                if (dailyCost > 0.01) {
                                    std::ostringstream s;

                                    // Format column X as money
                                    s << std::fixed << std::setprecision(2) << dailyCost;
                                    dailyCostString = s.str();
                                    
                                    // Column Y
                                    s.str(std::string()); // clear the variable s
                                    s // << std::fixed << std::setprecision(2)
                                    << "(" << package_cost << " / " << package_mg << ") * " << daily_dosage_mg;
                                    dailyCostFormula = s.str();
                                }
#ifdef DEBUG
                                std::clog
                                << "gtin13 " << pv.gtin13
                                << ", rn5 " << pv.rn5
                                << ", atc " << atc
                                << ", daily_dosage_mg " << daily_dosage_mg
                                << ", dailyCost <" << dailyCost << ">"
                                << std::endl;
#endif
                            }
                        }
                    }
                }
            }
        }

        ofs
        << CELL_ESCAPE << pv.rn5 << CELL_ESCAPE << OUTPUT_FILE_SEPARATOR              // A
        << CELL_ESCAPE << pv.code3 << CELL_ESCAPE << OUTPUT_FILE_SEPARATOR            // B
        << CELL_ESCAPE << pv.rn5 << pv.code3 << CELL_ESCAPE << OUTPUT_FILE_SEPARATOR  // C
        << CELL_ESCAPE << pv.gtin13 << CELL_ESCAPE << OUTPUT_FILE_SEPARATOR           // D
        << CELL_ESCAPE << name << CELL_ESCAPE << OUTPUT_FILE_SEPARATOR                // E
        << pv.galenicForm << OUTPUT_FILE_SEPARATOR                      // F
        << dosage << OUTPUT_FILE_SEPARATOR                              // G
        << pv.du.dosage << " " << pv.du.units << OUTPUT_FILE_SEPARATOR  // H
        << calculatedDosageString << OUTPUT_FILE_SEPARATOR              // I
        << fromBag.efp << OUTPUT_FILE_SEPARATOR                         // J
        << fromBag.pp << OUTPUT_FILE_SEPARATOR                          // K
        << pv.owner << OUTPUT_FILE_SEPARATOR                            // L
        << pv.categoryPack << OUTPUT_FILE_SEPARATOR                     // M
        << bagFlagSL << OUTPUT_FILE_SEPARATOR                           // N
        << fromBag.efp_validFrom << OUTPUT_FILE_SEPARATOR               // O
        << pv.regDate << OUTPUT_FILE_SEPARATOR                          // P
        << pv.validUntil << OUTPUT_FILE_SEPARATOR                       // Q
        << auth << OUTPUT_FILE_SEPARATOR                                // R
#if 1
        << bagFlagGeneric << OUTPUT_FILE_SEPARATOR                      // S
#else
        << boost::algorithm::join(fromBag.flags, ",") << OUTPUT_FILE_SEPARATOR // S
#endif
        << itCodeBag << OUTPUT_FILE_SEPARATOR                           // T
        << pv.itNumber << OUTPUT_FILE_SEPARATOR                         // U
        << pv.narcoticFlag << OUTPUT_FILE_SEPARATOR                     // V
        << pv.categoryMed << OUTPUT_FILE_SEPARATOR                      // W
        << CELL_ESCAPE << dailyCostString << CELL_ESCAPE << OUTPUT_FILE_SEPARATOR // X
        << dailyCostFormula                                             // Y
        << std::endl;
    }
    
    ofs.close();
    
    std::clog << std::endl << "Created " << filename << std::endl;
}
}
