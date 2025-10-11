//
//  swissreg.hpp
//  swissreg
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 11 Oct 2025
//
// https://github.com/zdavatz/cpp2sqlite/issues/264

#ifndef swissreg_hpp
#define swissreg_hpp

#include <nlohmann/json.hpp>

namespace SWISSREG
{
    class Certificate {
    public:
        std::string certificateId;
        std::string certificateNumber;
        std::string issueDate;
        std::string publicationDate;
        std::string registrationDate;
        std::string protectionDate;
        std::string basePatentDate;
        std::string basePatent;
        // std::string basePatentId;
        std::vector<std::string> iksnrs;
        std::string expiryDate;
        std::string deletionDate;
    };
    void parseJSON(const std::string &filename);
    Certificate jsonToCertificate(std::string id, nlohmann::json entry);
    std::vector<Certificate> getCertsByRegnr(std::string regnr);
    static void printFileStats(const std::string &filename);
}

#endif /* swissreg_hpp */
