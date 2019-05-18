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

// Issue #70
//#define SAPPINFO_OLD_STATS
#define SAPPINFO_NEW_STATS

namespace SAPP
{
    struct _common {
        std::string atcCodes;    // could be a single ATC, or a comma separated list
        std::vector<std::string> atcCodeVec;  // break it down into individual ATCs
#define ATC_LIST_SEPARATOR  ", "

        std::string activeSubstance;    // Wirkstoff
        std::string mainIndication;     // Hauptindikation
        std::string indication;         // Indikation
        std::string typeOfApplication;  // Applikationsart
        std::string link;               // SAPP-Monographie
        std::string comments;           // Bemerkungen zur Dosierung
    };
    
    struct _breastfeed {
        _common c;
        std::string maxDailyDose;       // max. verabreichte Tagesdosis
        std::string approval;           // Zulassungsnummer
    };

    struct _pregnancy {
        _common c;
        std::string max1;        // max. verabreichte Tagesdosis 1. Trimenon
        std::string max2;        // max. verabreichte Tagesdosis 2. Trimenon
        std::string max3;        // max. verabreichte Tagesdosis 3. Trimenon
        //std::string Dosis;     // Dosisanpassung
        std::string periDosi;    // Peripartale Dosierung
        std::string periBeme;    // Bemerkungen zur peripartalen Dosierung
    };

    void parseXLXS(const std::string &inDir,
                   const std::string &filename,
                   const std::string &language);

    void getHtmlByAtc(const std::string atc,
                      std::string &htmlPregnancy,
                      std::string &htmlBreastfeed);
    
    void printUsageStats();
}

#endif /* sappinfo_hpp */
