//
//  drugshortage.hpp
//  drugshortage
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 9 Jan 2021
//
// https://github.com/zdavatz/cpp2sqlite/issues/140

#ifndef drugshortage_hpp
#define drugshortage_hpp

#include <map>
#include <nlohmann/json.hpp>

namespace DRUGSHORTAGE
{
    class DrugShortage {
    public:
        int64_t gtin;
        std::string status;
        std::string estimatedDateOfDelivery; // datumLieferfahigkeit
        std::string dateLastMutation; // datumLetzteMutation
        int64_t colourCode;
    };
    void parseJSON (const std::string &filename, const std::string &inDir, const std::string &language);
    
    DrugShortage getEntryByGtin(int64_t gtin);
    DrugShortage jsonEntryToDrugShortage(nlohmann::json entry, std::string language);
    static void printFileStats(const std::string &filename, std::string language);
    static std::string stringForColourCode(int64_t number, std::string language);

    std::string getLocalized(const std::string &language,
                             const std::string &s);
    void getDeeplTranslationMap(const std::string &dir,
                            const std::string &job,
                            const std::string &language,
                            std::map<std::string, std::string> &map);
}

#endif /* drugshortage_hpp */
