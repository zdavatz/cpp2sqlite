//
//  peddose.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 15 Feb 2019
//

#ifndef peddose_hpp
#define peddose_hpp

namespace PED
{
    struct _case {
        std::string caseId;
        std::string atcCode;
        std::string indicationKey;
        std::string RoaCode;
    };

    struct _indication {
        std::string name;
        std::string recStatus;
    };

    struct _code {
        std::string value;
        std::string description;
        std::string recStatus;
    };

    struct _dosage {
        std::string key;

        std::string ageFrom;
        std::string ageFromUnit;

        std::string ageTo;
        std::string ageToUnit;

        std::string ageWeightRelation;

        std::string weightFrom;
        std::string weightTo;

        std::string doseLow;
        std::string doseHigh;
        std::string doseUnit;
        std::string doseUnitRef1;
        std::string doseUnitRef2; // <DoseRangeReferenceUnit2>

        std::string dailyRepetitionsLow;
        std::string dailyRepetitionsHigh;

        std::string maxSingleDose;
        std::string maxSingleDoseUnit;
        std::string maxSingleDoseUnitRef1;
        std::string maxSingleDoseUnitRef2;

        std::string maxDailyDose;
        std::string maxDailyDoseUnit;
        std::string maxDailyDoseUnitRef1;
        std::string maxDailyDoseUnitRef2;

        std::string roaCode;
        std::string remarks;

        std::string caseId;
        std::string type;
    };
    
    void parseXML(const std::string &filename,
                  const std::string &language);

    std::string getTextByAtcs(const std::string atcs);
    void getCasesByAtc(const std::string &atc, std::vector<_case> &cases);
    std::string getDescriptionByAtc(const std::string &atc);
    std::string getIndicationByKey(const std::string &key);

    void getDosageById(const std::string &id, std::vector<_dosage> &dosages);
    

    //std::string getRoaDescription(const std::string &codeValue);

    std::string getHtmlByAtc(const std::string atc);
    void showPedDoseByAtc(const std::string atc);

    void printUsageStats();
}

#endif /* peddose_hpp */
