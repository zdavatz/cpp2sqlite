//
//  main.cpp
//  sappinfo
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 3 May 2019
//
// Purpose:
//  Extract German strings from the input file and
//  write them out to an output file to be fed separately to
//  the translation engine DeepL

#include <iostream>
#include <string>
#include <set>
#include <unordered_set>
#include <libgen.h>     // for basename()

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string_regex.hpp>

#include <xlnt/xlnt.hpp>

#include "config.h"

// Columns that don't contain text are commented out (except for the filter column)

#define COLUMN_B        1   // Hauptindikation
#define COLUMN_C        2   // Indikation
#define COLUMN_G        6   // Wirkstoff
#define COLUMN_H        7   // Applikationsart
#define COLUMN_I        8   // max. verabreichte Tagesdosis
#define COLUMN_J        9   // Bemerkungen zur Dosierung
//#define COLUMN_Q       16   // Zulassungsnummer
//#define COLUMN_R       17   // ATC
//#define COLUMN_S       18   // SAPP-Monographie
#define COLUMN_U       20   // Filter

#define COLUMN_2_B      1   // Hauptindikation
#define COLUMN_2_C      2   // Indikation
#define COLUMN_2_G      6   // Wirkstoff
#define COLUMN_2_H      7   // Applikationsart
#define COLUMN_2_I      8   // max. verabreichte Tagesdosis 1. Trimenon
#define COLUMN_2_J      9   // max. verabreichte Tagesdosis 2. Trimenon
#define COLUMN_2_K     10   // max. verabreichte Tagesdosis 3. Trimenon
#define COLUMN_2_L     11   // Bemerkungen zur Dosierung
//#define COLUMN_2_M     12   // Peripartale Dosierung
#define COLUMN_2_N     13   // Bemerkungen zur peripartalen Dosierung
//#define COLUMN_2_Z     25   // ATC
//#define COLUMN_2_AA    26   // SAPP-Monographie
#define COLUMN_2_AC    28   // Filter

#define FIRST_DATA_ROW_INDEX    1

namespace po = boost::program_options;
static std::string appName;

#pragma mark - DEEPL

namespace DEEPL
{
std::set<std::string> toBeTranslatedSet; // no duplicates, sorted

// See also src/c2s/sappinfo.cpp getLocalized()
void validateAndAdd(const std::string s)
{
    if (s.empty())
        return;
    
    // Skip if it starts with a number
    // sheet 1, column I, could be like "120mg" or "keine Angaben"
    if (std::isdigit(s[0]))
        return;
    
    // Sometimes it start with a number, but after a space: " 80mg"
    // example sheet 2, column K, ATC C05AD01
    if (std::isspace(s[0]) && std::isdigit(s[1]))
        return;

    // Treat this as an empty cell
    if (s == "-")
        return;

    toBeTranslatedSet.insert(s);
}

void parseXLXS(const std::string &inFilename,
               const std::string &outDir,
               bool verbose)
{
    const std::unordered_set<int> acceptedFiltersSet = { 1, 5, 6, 9 };
    std::clog << std::endl << "Reading " << inFilename << std::endl;
    xlnt::workbook wb;
    wb.load(inFilename);
    
    //--- Breast-feeding sheet -------------------------------------------------
    const std::string title1("Stillzeit");
    if (wb.contains(title1)) {
        auto ws = wb.sheet_by_title(title1);

        std::clog << "Sheet title: " << ws.title() << std::endl;
        
        int skipHeaderCount = 0;
        for (auto row : ws.rows(false)) {
            if (++skipHeaderCount <= FIRST_DATA_ROW_INDEX) {
                continue;
            }
            
            if (row[COLUMN_U].to_string().empty())
                continue;                // Issue #85
            
            int filter = std::stoi(row[COLUMN_U].to_string());
            if (acceptedFiltersSet.find(filter) == acceptedFiltersSet.end())
                continue;            // Not found in set
            
            std::vector<std::string> aSingleRow;
            for (auto cell : row)
                aSingleRow.push_back(cell.to_string());

            const std::unordered_set<int> columnsWithTextSet = {
                COLUMN_B,   // Hauptindikation
                COLUMN_C,   // Indikation
                COLUMN_G,   // Wirkstoff
                COLUMN_H,   // Applikationsart
                COLUMN_J,   // Bemerkungen zur Dosierung
                
                // Sometimes dosage, other times text:
                COLUMN_I   // max. verabreichte Tagesdosis
            };
            
            for (auto s : columnsWithTextSet)
                validateAndAdd(aSingleRow[s]);
        } // for row in sheet 1
    }

    //--- Pregnancy sheet ------------------------------------------------------
    const std::string title2("Schwangerschaft");
    if (wb.contains(title2)) {
        auto ws = wb.sheet_by_title(title2);
        std::clog << "Sheet title: " << ws.title() << std::endl;
        
        int skipHeaderCount = 0;
        for (auto row : ws.rows(false)) {
            if (++skipHeaderCount <= FIRST_DATA_ROW_INDEX) {
                continue;
            }
            
            if (row[COLUMN_2_AC].to_string().empty()) {
                // Issue #64
                // The last line of the second sheet contains just one subtotal for G
                // The filter column is empty, so just ignore it
                continue;
            }

            int filter = std::stoi(row[COLUMN_2_AC].to_string());
            if (acceptedFiltersSet.find(filter) == acceptedFiltersSet.end())
                continue;            // Not found in set
            
            std::vector<std::string> aSingleRow;
            for (auto cell : row)
                aSingleRow.push_back(cell.to_string());

            const std::unordered_set<int> columnsWithTextSet = {
                COLUMN_2_B, // Hauptindikation
                COLUMN_2_C, // Indikation
                COLUMN_2_G, // Wirkstoff
                COLUMN_2_H, // Applikationsart
                COLUMN_2_L, // Bemerkungen zur Dosierung
                COLUMN_2_N, // Bemerkungen zur peripartalen Dosierung

                // Sometimes dosage, other times text:
                COLUMN_2_I, // max. verabreichte Tagesdosis 1. Trimenon
                COLUMN_2_J, // max. verabreichte Tagesdosis 2. Trimenon
                COLUMN_2_K  // max. verabreichte Tagesdosis 3. Trimenon
            };

            for (auto s : columnsWithTextSet)
                validateAndAdd(aSingleRow[s]);
        } // for row in sheet 2
    }
}

} // namespace DEEPL

#pragma mark -

void on_version()
{
    std::cout << appName << " " << PROJECT_VER
    << ", " << __DATE__ << " " << __TIME__ << std::endl;
    
    std::cout << "C++ " << __cplusplus << std::endl;
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

    DEEPL::parseXLXS(opt_inputDirectory + "/sappinfo.xlsx",
                     opt_workDirectory + "/output",
                     false);

#ifdef DEBUG
    std::clog << basename((char *)__FILE__) << ":" << __LINE__
    << ", toBeTranslatedSet size: " << DEEPL::toBeTranslatedSet.size()
    << std::endl;
#endif

    {
        std::string toBeTran = boost::algorithm::join(DEEPL::toBeTranslatedSet, "\n");
        std::ofstream outfile(opt_inputDirectory + "/deepl.sappinfo.in.txt");
        outfile << toBeTran;
        outfile.close();
    }
    
    return EXIT_SUCCESS;
}
