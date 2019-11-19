//
//  kunden.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 19 Nov 2019
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <boost/algorithm/string.hpp>

// For "typed" JSON
#include <nlohmann/json.hpp>

#include <libgen.h>     // for basename()

#include "neu.hpp"

namespace NEU
{
constexpr std::string_view CSV_SEPARATOR = ";";

struct pharmaGroup {
    int helvepharm;
    int mepha;
    int sandoz;
    int spirig;
};

struct User {
    std::string rose_id;    // A
    std::string gln_code;   // B
    pharmaGroup neu_map;    // C,D,E,F
    std::string name1;      // G        Company name 1
    std::string street;     // H
    std::string zip;        // I
    std::string city;       // J
};

std::map<std::string, User> user_map;

// Parse-phase stats
unsigned int statsCsvLineCount = 0;

static
int getCellValue(const std::string &cell)
{
    if (boost::contains(cell, "Keine"))
        return 0;

    return 1;
}

// The file starts with BOM "EF BB BF"
void parseCSV(const std::string &filename, bool dumpHeader)
{
    std::clog << std::endl << "Reading " << filename << std::endl;
    
    try {
         std::ifstream file(filename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str))
        {
            boost::algorithm::trim_right_if(str, boost::is_any_of(" \n\r"));
            statsCsvLineCount++;
            
            if (header) {
                header = false;

                if (dumpHeader) {
                    std::ofstream outHeader(filename + ".header.txt");
                    std::vector<std::string> headerTitles;
                    boost::algorithm::split(headerTitles, str, boost::is_any_of(CSV_SEPARATOR));
                    outHeader << "Number of columns: " << headerTitles.size() << std::endl;
                    auto colLetter1 = ' ';
                    auto colLetter2 = 'A';
                    int index = 0;
                    for (auto t : headerTitles) {
                        outHeader << index++ << ") " << colLetter1 << colLetter2 << "\t" << t << std::endl;
                        if (colLetter2++ == 'Z') {
                            colLetter2 = 'A';
                            if (colLetter1 == ' ')
                                colLetter1 = 'A';
                            else
                                colLetter1++;
                        }
                    }

                    outHeader.close();
                }

                continue;
            }
            
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(CSV_SEPARATOR));

            if (columnVector.size() != 10) {
#ifdef DEBUG
                std::clog
                << "CSV Line " << statsCsvLineCount
                << ", unexpected # columns: " << columnVector.size() << std::endl;
#endif
//                statsLinesWrongNumFields.push_back(statsCsvLineCount);
                
                // Try to recover as much as possible: assume column A and D are correct
//                roseid_to_gln_map.insert(std::make_pair(columnVector[0], columnVector[3]));

                continue;
            }

            std::string gln_code = columnVector[1];    // B
#ifdef DEBUG
            if (gln_code.length() == 0) {
                std::cerr << __LINE__ << " statsLinesEmptyGlnVec "
                << ", CSV Line " << statsCsvLineCount
                << std::endl;
                //statsLinesEmptyGlnVec.push_back(std::to_string(statsCsvLineCount));
            }
            
            if (gln_code == "o z") {
                std::cerr << __LINE__ << " statsLinesGlnIsOz "
                << ", CSV Line " << statsCsvLineCount
                << std::endl;
                // We can still use it, just report it
                //statsLinesGlnIsOz.push_back(statsCsvLineCount);
            }
#endif
            
            User user;
            user.rose_id = columnVector[0];     // A
            user.gln_code = gln_code;

            user.neu_map = {
                getCellValue(columnVector[5]),      // F
                getCellValue(columnVector[2]),      // C
                getCellValue(columnVector[3]),      // D
                getCellValue(columnVector[4])};     // E

            user.name1 = columnVector[6];    // G        Company name 1
            user.street = columnVector[7];    // H
            user.zip = columnVector[8];    // I
            user.city = columnVector[9];    // J
            
#ifdef DEBUG
            if (user_map.find(user.rose_id) != user_map.end())
                std::cerr << __LINE__ << " doctor exists " << user.rose_id
                << ", CSV Line " << statsCsvLineCount
                << std::endl;
#endif
            user_map.insert(std::make_pair(user.rose_id, user));

        } // while
     
        file.close();
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
#ifdef DEBUG
    std::clog
    << "Parsed " << user_map.size() << " doctors" << std::endl
    << "CSV Lines " << statsCsvLineCount
//    << ", bad line(s):" << statsLinesWrongNumFields.size()
    << std::endl;
#endif
}

// Each entry in the JSON file is listed by Knr
void createConditionsNewJSON(const std::string &filename)
{
    std::clog << "Writing " << filename << std::endl;
    
    nlohmann::json tree;

    for (auto u : user_map)
    {
        nlohmann::json child;
        
        nlohmann::json mapChild;
        mapChild["helvepharm"] = u.second.neu_map.helvepharm;
        mapChild["mepha"] = u.second.neu_map.mepha;
        mapChild["sandoz"] = u.second.neu_map.sandoz;
        mapChild["spirig"] = u.second.neu_map.spirig;
        child["neu_map"] = mapChild;
        
        child["gln_code"] = u.second.gln_code;
        child["name1"] = u.second.name1;
        child["street"] = u.second.street;
        child["zip"] = u.second.zip;
        child["city"] = u.second.city;

        tree[u.first] = child;
    }
    
    std::ofstream out(filename);
    out << tree.dump(1, '\t');          // UTF-8
    //out << tree.dump(1, '\t', true);  // ASCII with escaped UTF-8 characters, like Java
    out.close();
}

}
