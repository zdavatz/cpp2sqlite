//
//  sappinfo.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 26 Aor 2019
//

#ifndef sappinfo_hpp
#define sappinfo_hpp

namespace SAPP
{
    struct _common {
        std::string atcCode;
        std::string activeSubstance;    // Wirkstoff
        std::string mainIndication;     // Hauptindikation
        std::string indication;         // Indikation
        std::string typeOfApplication;  // Applikationsart

    };
    
    struct _breastfeed {
        _common c;
        std::string maxDailyDose;        // max. verabreichte Tagesdosis
    };

    struct _pregnancy {
        _common c;
        std::string comments;        // Bemerkungen
    };

    void parseXLXS(const std::string &filename,
                   const std::string &language);
    std::string getHtmlByAtc(const std::string atc);
    
    void printUsageStats();
    
//private:
    static void getBreastFeedByAtc(const std::string &atc, std::vector<_breastfeed> &bfv);
    static void getPregnancyByAtc(const std::string &atc, std::vector<_pregnancy> &pv);
}

#endif /* sappinfo_hpp */
