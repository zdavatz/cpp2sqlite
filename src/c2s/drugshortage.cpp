//
//  drugshortage.cpp
//  drugshortage
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 9 Jan 2021
//
// https://github.com/zdavatz/cpp2sqlite/issues/140


#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <libgen.h>     // for basename()

#include <boost/algorithm/string.hpp>
#include "drugshortage.hpp"
#include "report.hpp"

namespace DRUGSHORTAGE
{
    std::map<int64_t, DrugShortage> drugshortageMap;
    std::map<int64_t, int64_t> colourCodeCounts;
    std::map<std::string, std::string> deeplTranslatedMap;
    
    void parseJSON (const std::string &filename, const std::string &inDir, const std::string &language) {
        if (language != "de")
            getDeeplTranslationMap(inDir, "drugshortage", language, deeplTranslatedMap);

        try {
            std::clog << std::endl << "Reading " << filename << std::endl;

            std::ifstream jsonInputStream(filename);
            nlohmann::json drugshortageJson;
            jsonInputStream >> drugshortageJson;
            for (nlohmann::json::iterator it = drugshortageJson.begin(); it != drugshortageJson.end(); ++it) {
                auto entry = it.value();
                auto drugshortage = jsonEntryToDrugShortage(entry, language);
                drugshortageMap[drugshortage.gtin] = drugshortage;
            }
        }
        catch (std::exception &e) {
            std::cerr
            << basename((char *)__FILE__) << ":" << __LINE__
            << " Error " << e.what()
            << std::endl;
        }
        printFileStats(filename, language);
    }

    DrugShortage jsonEntryToDrugShortage(nlohmann::json entry, std::string language) {
        DrugShortage d;
        d.gtin = entry["gtin"].get<int64_t>();
        d.status = getLocalized(language, entry["status"].get<std::string>());
        d.estimatedDateOfDelivery = getLocalized(language, entry["datumLieferfahigkeit"].get<std::string>());
        d.dateLastMutation = getLocalized(language, entry["datumLetzteMutation"].get<std::string>());
        try {
            int colourCode = entry["colorCode"]["#"].get<int64_t>();
            d.colourCode = colourCode;
        } catch(...){}
        return d;
    }
    
    DrugShortage getEntryByGtin(int64_t gtin) {
        if (drugshortageMap.find(gtin) != drugshortageMap.end()) {
            return drugshortageMap[gtin];
        }
        DrugShortage d;
        d.gtin = 0;
        return d;
    }

    static void printFileStats(const std::string &filename, std::string language)
    {
        REP::html_h2("Drugshortage");

        REP::html_p(filename);
        
        REP::html_start_ul();
        REP::html_li("GTINs : " + std::to_string(drugshortageMap.size()));
        for (int i = 1; i <= 5; i++) {
            int64_t count = colourCodeCounts.find(i) != colourCodeCounts.end() ? colourCodeCounts[i] : 0;
            REP::html_li(stringForColourCode(i, language) + "(" + std::to_string(i) + "): " + std::to_string(count));
        }
        REP::html_end_ul();
    }

    static std::string stringForColourCode(int64_t number, std::string language) {
        switch (number) {
            case 1:
                if (language == "fr") {
                    return "Total vert";
                }
                return "Total grün";
            case 2:
                if (language == "fr") {
                    return "Total vert clair";
                }
                return "Total hellgrün";
            case 3:
                if (language == "fr") {
                    return "Total orange";
                }
                return "Total orange";
            case 4:
                if (language == "fr") {
                    return "Total rouge";
                }
                return "Total rot";
            case 5:
                if (language == "fr") {
                    return "Total Jaune";
                }
                return "Total Gelb";
            default:
                return "";
        }
    }

    std::string getLocalized(const std::string &language,
                             const std::string &s)
    {
        if (language == "de")
            return s;
        
        if (s.empty())
            return s;

        // Treat this as an empty cell
        if (s == "-")
            return {};
        if (deeplTranslatedMap.find(s) == deeplTranslatedMap.end()) {
            std::clog << "No translation for <" << s << ">" << std::endl;
            return s;
        }
        auto translated = deeplTranslatedMap[s];
        if (translated.empty()) {
            return s;
        }
        return translated;
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
            while (std::getline(ifsKey, key))
            {
                std::getline(ifsValue, val);
    #ifdef WITH_DEEPL_MANUALLY_TRANSLATED
                if (val.empty())                    // DeepL failed to translate it
                    std::getline(ifsValue2, val);   // Get it from manually translated file
    #endif
    #ifdef DEBUG
                assert(!val.empty());
    #endif

                map.insert(std::make_pair(key, val));
            } // while
            
            ifsKey.close();
            ifsValue.close();
    #ifdef WITH_DEEPL_MANUALLY_TRANSLATED
            ifsValue2.close();
    #endif
        }
        catch (std::exception &e) {
            std::cerr
            << basename((char *)__FILE__) << ":" << __LINE__
            << " Error " << e.what()
            << std::endl;
        }
    }
}