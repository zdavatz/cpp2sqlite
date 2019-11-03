//
//  kunden.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 1 Nov 2019
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <boost/algorithm/string.hpp>

#include <libgen.h>     // for basename()

#include "kunden.hpp"

namespace KUNDEN
{

struct User {
    std::string gln_code;   // D
    std::string name1;      // O
    std::string street;     // R
    std::string zip;        // T
    std::string city;       // U
    std::string email;      // V
    std::string special_rebate; // AC
    bool top_customer;      // AD
    std::string revenue;    // AE Jahres Umsatz = annual turnover
};

std::map<std::string, User> user_map;

// Parse-phase stats
unsigned int statsCsvNumLines = 0;

void parseCSV(const std::string &filename)
{
    std::clog << "Reading " << filename << std::endl;

    try {
        std::ifstream file(filename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str))
        {
            statsCsvNumLines++;

            if (header) {
                header = false;
#ifdef DEBUG
                std::vector<std::string> headerTitles;
                boost::algorithm::split(headerTitles, str, boost::is_any_of(";"));
                std::clog << "Number of columns: " << headerTitles.size() << std::endl;
                auto colLetter = 'A';
                for (auto t : headerTitles)
                    std::clog << colLetter++ << "\t" << t << std::endl;
#endif
                continue;
            }
            
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(";"));
            
            if (columnVector.size() != 31) {
                std::clog
                << "Line " << statsCsvNumLines
                << ", unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }
            
            User user;
            user.gln_code = columnVector[3];    // D
            user.name1 = columnVector[14];      // O
            user.street = columnVector[17];     // R
            user.zip = columnVector[19];        // T
            user.city = columnVector[20];       // U
            user.email = columnVector[21];      // V
            user.top_customer = (boost::to_lower_copy<std::string>(columnVector[29]) == "true"); // AD

            user.special_rebate = columnVector[28]; // AC
            user.revenue = columnVector[30];        // AE
            
            // TODO:
            
            user_map.insert(std::make_pair(user.gln_code, user));

        } // while
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
#ifdef DEBUG
    std::clog
    << "Parsed " << user_map.size() << " doctors"
    << std::endl;
#endif
}

// See ShoppingCartRose.java 194
void createConditionsJSON(const std::string &filename)
{
    std::clog << "TODO Writing " << filename << std::endl;
    // TODO: write out user_map
}

// See ShoppingCartRose.java 206
void createIdsJSON(const std::string &filename)
{
    std::clog << "TODO Writing " << filename << std::endl;
    // TODO: write out roseid_to_gln_map
}

}
