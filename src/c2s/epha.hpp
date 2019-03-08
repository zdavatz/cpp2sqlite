//
//  epha.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 31 Jan 2019
//

#ifndef epha_hpp
#define epha_hpp

namespace EPHA
{
    struct Document {
        // zulassung doesn't belong here because it is the key for the map

        std::string substance; // substanz
        std::string manufacturer; // hersteller
        std::string atc;
        std::string product; // produkt
    };
    
    typedef std::vector<Document> DocumentList;

    void parseJSON(const std::string &filename, bool verbose);
    std::string getAtcFromSingleRn(const std::string &rn);
}

#endif /* epha_hpp */
