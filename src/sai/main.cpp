//
//  main.cpp
//  sappinfo
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 11 May 2021
//
// Purpose:
// https://github.com/zdavatz/cpp2sqlite/issues/181

#include <iostream>
#include <string>
#include <set>
#include <unordered_set>
#include <libgen.h>     // for basename()

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string_regex.hpp>

#include <xlnt/xlnt.hpp>

#include <sqlite3.h>
#include "report.hpp"
#include "sqlDatabase.hpp"

#include "config.h"
#include "sai.hpp"
#include "praeparate.hpp"
#include "sequenzen.hpp"
#include "stoffsynonyme.hpp"
#include "deklarationen.hpp"
#include "adressen.hpp"

constexpr std::string_view TABLE_NAME_SAI = "sai_db";

namespace po = boost::program_options;
static std::string appName;

static DB::Sql sqlDb;

void on_version()
{
    std::cout << appName << " " << PROJECT_VER
    << ", " << __DATE__ << " " << __TIME__ << std::endl;
    
    std::cout << "C++ " << __cplusplus << std::endl;
}

void openDB(const std::string &filename)
{
    std::clog << std::endl << "Create DB: " << filename << std::endl;

    sqlDb.openDB(filename);
    sqlDb.createTable(TABLE_NAME_SAI, "_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "zulassungsnummer TEXT, "
        "sequenznummer TEXT, "
        "packungscode TEXT, "
        "zulassungsstatus TEXT, "
        "bemerkung_freitext TEXT, "
        "packungsgroesse TEXT, "
        "packungseinheit TEXT, "
        "widerruf_verzicht_datum TEXT, "
        "btm_code TEXT, "
        "gtin_industry TEXT, "
        "im_handel_datum_industry TEXT, "
        "ausser_handel_datum_industry TEXT, "
        "beschreibung_de_refdata TEXT, "
        "beschreibung_fr_refdata TEXT, "

        // from SAI-Praeparate.XML
        "verwendung TEXT,"
        "praeparatename TEXT,"
        "arzneiform TEXT,"
        "atc_code TEXT,"
        "heilmittel_code TEXT,"
        "zulassungskategorie TEXT,"
        "zulassungsinhaberin TEXT,"
        "erstzulassungsdatum TEXT,"
        "basis_zulassungsnummer TEXT,"
        "abgabekategorie TEXT,"
        "it_nummer TEXT,"
        "anwendungsgebiet TEXT,"
        "ablaufdatum TEXT,"
        "ausstellungsdatum TEXT,"
        "chargenblockade_aktiv TEXT,"
        "chargenfreigabe_pflicht TEXT,"
        "einzeleinfuhr_bewillig_pflicht TEXT,"
        "ocabr_standard_common_name TEXT, "

        // from SAI-Sequenzen.XML
        "sequenzname TEXT, "
        "zulassungsart TEXT, "
        "basis_sequenznummer TEXT, "

        // from SAI-Deklarationen.XML
        "zusammensetzung TEXT, "

        "firmenname TEXT, "
        "gln_refdata TEXT"
    );
    sqlDb.createIndex(TABLE_NAME_SAI, "idx_", {"zulassungsnummer", "sequenznummer", "packungscode"});
    sqlDb.prepareStatement(TABLE_NAME_SAI,
                           "null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?");
}

void closeDB()
{
    sqlDb.destroyStatement();
    sqlDb.closeDB();
}

int main(int argc, char **argv)
{
    appName = boost::filesystem::basename(argv[0]);
    
    std::string opt_inputDirectory;
    std::string opt_workDirectory;  // for downloads subdirectory
    bool flagVerbose = false;
    
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "print this message")
    ("version,v", "print the version information and exit")
    ("verbose", "be extra verbose") // Show errors and logs
    ("inDir", po::value<std::string>( &opt_inputDirectory )->required(), "input directory") //  without trailing '/'
    ("workDir", po::value<std::string>( &opt_workDirectory ), "parent of 'downloads' and 'output' directories, default as parent of inDir ")
    ;
    
    po::variables_map vm;
    
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm); // populate vm
        
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }
        
        if (vm.count("version")) {
            on_version();
            return EXIT_SUCCESS;
        }
        
        po::notify(vm); // raise any errors encountered
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (po::error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Exception of unknown type!" << std::endl;
        return EXIT_FAILURE;
    }
    
    if (vm.count("verbose")) {
        flagVerbose = true;
    }
    
    if (!vm.count("workDir")) {
        opt_workDirectory = opt_inputDirectory + "/..";
    }
    
    // Parse input files

    std::string reportFilename("sai_report.html");
    std::string reportTitle("SAI Report");
    REP::init(opt_workDirectory + "/output/", reportFilename, reportTitle, false);

    REP::html_start_ul();
    for (int i=0; i<argc; i++)
        REP::html_li(argv[i]);
    
    REP::html_end_ul();

    // REP::html_h1("SAI-Packungen.XML");

    SAI::parseXML(opt_workDirectory + "/downloads/SAI/SAI-Packungen.XML");
    PRA::parseXML(opt_workDirectory + "/downloads/SAI/SAI-Praeparate.XML");
    SEQ::parseXML(opt_workDirectory + "/downloads/SAI/SAI-Sequenzen.XML");
    DEK::parseXML(opt_workDirectory + "/downloads/SAI/SAI-Deklarationen.XML");
    STO::parseXML(opt_workDirectory + "/downloads/SAI/SAI-Stoff-Synonyme.XML");
    ADR::parseXML(opt_workDirectory + "/downloads/SAI/SAI-Adressen.XML");

    std::string dbFilename = opt_workDirectory + "/output/sai.db";
    openDB(dbFilename);

    int i = 0;
    auto packages = SAI::getPackages();
    int total = packages.size();
    std::set<std::string> missingDeklarationen;
    for (auto package : packages) {
        std::cerr << "\r" << 100*i/total << " % ";
        sqlDb.bindText(1, package.approvalNumber);
        sqlDb.bindText(2, package.sequenceNumber);
        sqlDb.bindText(3, package.packageCode);
        sqlDb.bindText(4, package.approvalStatus);
        sqlDb.bindText(5, package.noteFreeText);
        sqlDb.bindText(6, package.packageSize);
        sqlDb.bindText(7, package.packageUnit);
        sqlDb.bindText(8, package.revocationWaiverDate);
        sqlDb.bindText(9, package.btmCode);
        sqlDb.bindText(10, package.gtinIndustry);
        sqlDb.bindText(11, package.inTradeDateIndustry);
        sqlDb.bindText(12, package.outOfTradeDateIndustry);
        sqlDb.bindText(13, package.descriptionEnRefdata);
        sqlDb.bindText(14, package.descriptionFrRefdata);

        PRA::_package praPackage;
        try {
            praPackage = PRA::getPackageByZulassungsnummer(package.approvalNumber);
        } catch (std::out_of_range e) {
            std::clog << "Not found praeparate: " << package.approvalNumber << std::endl;
        }

        sqlDb.bindText(15, praPackage.verwendung);
        sqlDb.bindText(16, praPackage.praeparatename);
        sqlDb.bindText(17, praPackage.arzneiform);
        sqlDb.bindText(18, praPackage.atcCode);
        sqlDb.bindText(19, praPackage.heilmittelCode);
        sqlDb.bindText(20, praPackage.zulassungskategorie);
        sqlDb.bindText(21, praPackage.zulassungsinhaberin);
        sqlDb.bindText(22, praPackage.erstzulassungsdatum);
        sqlDb.bindText(23, praPackage.basisZulassungsnummer);
        sqlDb.bindText(24, praPackage.abgabekategorie);
        sqlDb.bindText(25, praPackage.itNummer);
        sqlDb.bindText(26, praPackage.anwendungsgebiet);
        sqlDb.bindText(27, praPackage.ablaufdatum);
        sqlDb.bindText(28, praPackage.ausstellungsdatum);
        sqlDb.bindText(29, praPackage.chargenblockadeAktiv);
        sqlDb.bindText(30, praPackage.chargenfreigabePflicht);
        sqlDb.bindText(31, praPackage.einzeleinfuhrBewilligPflicht);
        sqlDb.bindText(32, "");

        SEQ::_package seqPackage;
        try {
            seqPackage = SEQ::getPackagesByZulassungsnummerAndSequenznummer(package.approvalNumber, package.sequenceNumber);
        } catch (std::out_of_range e) {
            std::clog << "Not found Sequenzen: " << package.approvalNumber << std::endl;
        }
        sqlDb.bindText(33, seqPackage.sequenzname);
        sqlDb.bindText(34, seqPackage.zulassungsart);
        sqlDb.bindText(35, seqPackage.basisSequenznummer);

        std::string zusammensetzungString;
        try {
            std::vector<DEK::_package> dekPackages = DEK::getPackagesByZulassungsnummerAndSequenznummer(package.approvalNumber, package.sequenceNumber);
            std::sort(dekPackages.begin(), dekPackages.end(),
                [](DEK::_package const &a, DEK::_package const &b) {
                    int a1 = std::stoi(a.zeilennummer);
                    int b1 = std::stoi(b.zeilennummer);
                    if (a1 != b1) {
                        return a1 < b1;
                    }
                    a1 = std::stoi(a.sortierungZeilennummer);
                    b1 = std::stoi(b.sortierungZeilennummer);
                    return a1 < b1;
                }
            );
            
            for (auto dekPackage : dekPackages) {
                try {
                    STO::_package stoPackage = STO::getPackageByStoffId(dekPackage.stoffId);
                    std::string thisString = 
                        dekPackage.zeilennummer + ";" +
                        stoPackage.stoffsynonym + ";" +
                        stoPackage.synonymCode + ";" +
                        dekPackage.menge + ";" +
                        dekPackage.mengenEinheit + ";" +
                        dekPackage.deklarationsart + ";" +
                        dekPackage.stoffkategorie + ";" +
                        dekPackage.komponente;
                    zusammensetzungString += thisString + "\n";
                } catch (std::out_of_range e) {
                    std::clog << "Not found stoff: " << dekPackage.stoffId << std::endl;
                }
            }
        } catch (std::out_of_range e) {
            missingDeklarationen.insert(package.approvalNumber);
        }
        sqlDb.bindText(36, zusammensetzungString);

        ADR::_package adrPackage;
        try {
            adrPackage = ADR::getPackageByPartnerNr(praPackage.zulassungsinhaberin);
        } catch (std::out_of_range e) {

        }
        sqlDb.bindText(37, adrPackage.firmenname);
        sqlDb.bindText(38, adrPackage.glnRefdata);

        sqlDb.runStatement(TABLE_NAME_SAI);
        i++;
    }
    std::cerr << "\r" << "100 % ";

    REP::html_h2("rows with no deklarationen: ");
    REP::html_start_ul();
    for (auto str : missingDeklarationen) {
        REP::html_li(str);
    }
    
    REP::html_end_ul();

    closeDB();
    
    return EXIT_SUCCESS;
}
