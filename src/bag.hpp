//
//  bag.hpp
//  cpp2sqlite, pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 23 Jan 2019
//

#ifndef bag_hpp
#define bag_hpp

#include <iostream>
#include <set>
#include <map>
#include "gtin.hpp"

namespace BAG
{
    struct packageFields {
        std::string efp;
        std::string efp_validFrom;
        std::string pp;
        std::vector<std::string> flags;
    };

    struct ItCode {
        // shortest
        std::string tindex;         // localized

        // longest
        std::string application;    // localized
        std::string longestItCode;
    };

    /// Indikationscode (BAG XXXXX.NN) — mandatory on prescriptions and
    /// invoices for SL drugs with a price model from 2026-07-01 (BAG
    /// Rundschreiben 2026-02-19).  XXXXX is the FOPHDossierNumber, NN is
    /// the trailing index from a sibling ClinicalUseDefinition's id.
    struct IndicationCode {
        std::string code;       // e.g. "18923.01"
        std::string cudId;      // raw CUD id (e.g. "CYRAMZA.01")
        std::string text;       // limitations text for this indication
    };

    struct Pack {
        std::string description;
        std::string descriptionDe;
        std::string descriptionFr;

        std::string category;
        std::string gtin;
        std::string exFactoryPrice;
        std::string exFactoryPriceValidFrom;
        std::string publicPrice;
        std::string publicPriceValidFrom;
        std::string limitationPoints;   // TODO
        std::string partnerDescription;
        std::string ggsl;
        std::vector<IndicationCode> indicationCodes;  // FHIR-only, see above
    };

    struct Preparation {
        std::string name;
        std::string nameDe;
        std::string nameFr;

        std::string description;
        std::string descriptionDe;
        std::string descriptionFr;

        std::string swissmedNo;     // same as regnr
        std::string atcCode;
        std::string orgen;
        int sb;
        std::vector<Pack> packs;
        ItCode itCodes;
        std::vector<IndicationCode> indicationCodes;  // FHIR-only, see above
    };

    typedef std::vector<Preparation> PreparationList;
    typedef std::map<std::string, packageFields> PackageMap;

    void parseXML(const std::string &filename,
                  const std::string &language,
                  bool verbose);

    int getAdditionalNames(const std::string &rn,
                           std::set<std::string> &gtinUsed,
                           GTIN::oneFachinfoPackages &packages);

    std::string getPricesAndFlags(const std::string &gtin,
                                  const std::string &fromSwissmedic,
                                  const std::string &category="");

    std::vector<std::string> getGtinList();
    std::string getTindex(const std::string &rn);
    std::string getApplicationByRN(const std::string &rn);
    std::string getLongestItCodeByGtin(const std::string &gtin);

    std::string formatPriceAsMoney(const std::string &price);

    packageFields makePackageFields(Preparation pre, Pack p);
    packageFields getPackageFieldsByGtin(const std::string &gtin);

    std::vector<std::string> gtinWhichDoesntStartWith7680();
    bool getPreparationAndPackageByGtin(const std::string &gtin, Preparation *outPrep, Pack *outPack);

    PreparationList getPrepList();

    void printUsageStats();
}

#endif /* bag_hpp */
