//
//  main.cpp
//  drugshortage
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 9 Jan 2021
//
// https://github.com/zdavatz/cpp2sqlite/issues/140

#include <iostream>
#include <string>
#include <regex>
#include <set>
#include <unordered_set>
#include <sqlite3.h>
#include <libgen.h>     // for basename()

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <nlohmann/json.hpp>
#include <xlnt/xlnt.hpp>

#include "config.h"

namespace po = boost::program_options;
static std::string appName;
std::string opt_language;
sqlite3 *db;

#pragma mark -

void on_version()
{
    std::cout << appName << " " << PROJECT_VER
    << ", " << __DATE__ << " " << __TIME__ << std::endl;

    std::cout << "SQLITE_VERSION: " << SQLITE_VERSION << std::endl;
    std::cout << "C++ " << __cplusplus << std::endl;
}

static std::map<int64_t, nlohmann::json> drugshortageJsonMap;

std::string updateChapterTitles(std::string chapterTitles) {
    std::string result = std::string(chapterTitles);
    if (opt_language == "de") {
        boost::replace_all(result, ";Packungen;", ";Packungen/Drugshortage;");
    } else {
        boost::replace_all(result, ";Présentation;", ";Présentation/Drugshortge;");
    }
    return result;
}

std::string updateContent(
    std::string contentHtml,
    std::vector<std::string> packageNames,
    std::vector<nlohmann::json> drugshortageJsonEntries
) {
    std::regex rgx;
    if (opt_language == "de") {
        rgx = R"(<div class=\"absTitle\">\s*Packungen\s*</div>)";    // tested at https://regex101.com
    } else {
        rgx = R"(<div class=\"absTitle\">\s*Présentation\s*</div>)";    // tested at https://regex101.com
    }
    std::smatch match;
    if (std::regex_search(contentHtml, match, rgx)) {
        int lastIndex = match.size() - 1;
        std::string matched = match[lastIndex];
        std::string before = contentHtml.substr(0, match.position(lastIndex));
        std::string after = contentHtml.substr(match.position(lastIndex) + match.length(lastIndex));
        if (opt_language == "de") {
            boost::replace_first(matched, "Packungen", "Packungen/Drugshortage");
        } else {
            boost::replace_first(matched, "Présentation", "Présentation/Drugshortge");
        }
        contentHtml = before + matched + after;
    }
    for (int i = 0; i < packageNames.size(); i++) {
        std::string packageName = packageNames[i];
        nlohmann::json jsonEntry = drugshortageJsonEntries[i];
        std::string extraString;
        if (jsonEntry.contains("status")) {
            extraString = "<p>Status: " + jsonEntry["status"].get<std::string>() + "</p>\n";
        }
        if (jsonEntry.contains("datumLieferfahigkeit")) {
            extraString += "<p>Geschaetztes Datum Lieferfaehigkeit: " + jsonEntry["datumLieferfahigkeit"].get<std::string>() + "</p>\n";
        }
        if (jsonEntry.contains("datumLetzteMutation")) {
            extraString += "<p>Datum Letzte Mutation: " + jsonEntry["datumLetzteMutation"].get<std::string>() + "</p>\n";
        }
        std::string searchString = "<p class=\"spacing1\">" + packageName;
        boost::replace_first(contentHtml, searchString, extraString + searchString);
    }
    return contentHtml;
}

void insertDrugStorage(
    sqlite3 *db, 
    std::string id, 
    std::string chapterTitles, 
    std::string htmlString, 
    std::vector<std::string> packageNames, 
    std::vector<nlohmann::json> drugshortageJsonEntries
) {
    std::string updatedTitles = updateChapterTitles(chapterTitles);
    std::string updatedContent = updateContent(htmlString, packageNames, drugshortageJsonEntries);
    sqlite3_stmt *statement = NULL;
    sqlite3_prepare_v2(db, "UPDATE amikodb SET titles_str = ?, content = ? WHERE _id = ? ", -1, &statement, NULL);
    sqlite3_bind_text(statement, 1, updatedTitles.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, updatedContent.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(statement);
    sqlite3_finalize(statement);
}

int onProcessRow(void *_processingGtin, int argc, char **argv, char **azColName) {
    int64_t processingGtin = (int64_t)_processingGtin;
    std::string packageStr = std::string(argv[17]);
    std::vector<std::string> lines;
    boost::algorithm::split(lines, packageStr, boost::is_any_of("\n"));
    std::vector<std::string> packageNames;
    std::vector<nlohmann::json> jsonEntries;
    for (std::string line : lines) {
        std::vector<std::string> parts;
        boost::algorithm::split(parts, line, boost::is_any_of("|"));
        std::string packageName = parts[0];
        std::string gtinString = parts[9];

        try {
            int64_t thisGtin = std::stoll(gtinString);
            if (thisGtin == processingGtin) {
                auto result = drugshortageJsonMap[thisGtin];
                std::clog << "Adding drug shortage: " << gtinString << std::endl;
                packageNames.push_back(packageName);
                jsonEntries.push_back(result);
            }
        } catch (...) {}
    }
    if (!jsonEntries.empty()) {
        insertDrugStorage(
            (sqlite3 *)db, 
            std::string(argv[0]), 
            std::string(argv[14]), 
            std::string(argv[15]), 
            packageNames,
            jsonEntries
        );
    }
    return 0;
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
    ("lang", po::value<std::string>( &opt_language )->default_value("de"), "use given language (de/fr)")
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

    std::string dbFilename = opt_workDirectory + "/output/amiko_db_full_idx_" + opt_language + ".db";
    
    int rc = sqlite3_open(dbFilename.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", error " << rc
        << std::endl;
        
        return 1;
    }
    char *errmsg;

    nlohmann::json drugshortageJson;
    std::string jsonFilename = opt_workDirectory + "/downloads/drugshortage.json";
    std::ifstream jsonInputStream(jsonFilename);
    jsonInputStream >> drugshortageJson;
    for (nlohmann::json::iterator it = drugshortageJson.begin(); it != drugshortageJson.end(); ++it) {
        auto entry = it.value();
        int64_t thisGtin = entry["gtin"].get<int64_t>();
        drugshortageJsonMap[thisGtin] = entry;
        sqlite3_exec(db, ("select * from amikodb where packages LIKE '%|" + std::to_string(thisGtin) + "|%';").c_str(), onProcessRow, (void *)thisGtin, &errmsg);
    }

    sqlite3_close(db);
    
    return EXIT_SUCCESS;
}
