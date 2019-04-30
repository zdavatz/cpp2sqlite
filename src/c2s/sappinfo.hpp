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
        std::string comments;           // Bemerkungen zur Dosierung
        std::string maxDailyDose;       // max. verabreichte Tagesdosis
    };

    struct _pregnancy {
        _common c;
        std::string link;        // SAPP-Monographie
        std::string max1;        // max. verabreichte Tagesdosis 1. Trimenon
        std::string max2;        // max. verabreichte Tagesdosis 2. Trimenon
        std::string max3;        // max. verabreichte Tagesdosis 3. Trimenon
        //std::string Dosis;     // Dosisanpassung
        std::string periDosi;    // Peripartale Dosierung
        std::string periBeme;    // Bemerkungen zur peripartalen Dosierung
    };

    void parseXLXS(const std::string &filename,
                   const std::string &language);
    std::string getHtmlByAtc(const std::string atc);
    
    void printUsageStats();
}

#endif /* sappinfo_hpp */
