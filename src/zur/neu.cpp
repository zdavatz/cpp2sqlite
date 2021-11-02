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

// For "all-string" JSON
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

// For "typed" JSON
#include <nlohmann/json.hpp>

#include <libgen.h>     // for basename()

#include "neu.hpp"
#include "report.hpp"

namespace pt = boost::property_tree;

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
    std::string special_group; // whether the user is in medix_kunden.csv
};

std::map<std::string, User> user_map;
std::map<std::string, std::string> roseid_to_gln_map;

// Parse-phase stats
unsigned int statsCsvLineCount = 0;
unsigned int statsMedixCsvLineCount = 0;
std::vector<int> statsLinesWrongNumFields;
std::vector<std::string> statsLinesEmptyGlnVec; // strings so that they can be conveniently joined
std::vector<int> statsLinesGlnIsOz;
unsigned int statsGlnIsNotUniqueCount;

static
void printFileStats(const std::string &filename)
{
    REP::html_h2("KUNDEN NEU");
    //REP::html_p(std::string(basename((char *)filename.c_str())));
    REP::html_p(filename);
    
    REP::html_start_ul();
    REP::html_li("Total Doctors: " + std::to_string(user_map.size()));
    REP::html_li("GLNs not unique: " + std::to_string(statsGlnIsNotUniqueCount));
    REP::html_end_ul();
    
    if (statsLinesWrongNumFields.size() > 0) {
        REP::html_p("Number of lines with wrong number of fields (skipped): " + std::to_string(statsLinesWrongNumFields.size()));
        REP::html_start_ul();
        for (auto s : statsLinesWrongNumFields)
            REP::html_li("Line " + std::to_string(s));
        
        REP::html_end_ul();
    }

    if (statsLinesEmptyGlnVec.size() > 0) {
        REP::html_p("Lines with empty 'GLN' (column D). Note that 'Knr Rose' (column A) will be used instead in 'rose_conditions_new.json'. Total count: " + std::to_string(statsLinesEmptyGlnVec.size()));

        if (statsLinesEmptyGlnVec.size() > 0)
            REP::html_div(boost::algorithm::join(statsLinesEmptyGlnVec, ", "));
    }
    
    if (statsLinesGlnIsOz.size() > 0) {
        REP::html_p("Number of lines with GLN set to 'o z' (test client, still used): " + std::to_string(statsLinesGlnIsOz.size()));
        REP::html_start_ul();
        for (auto s : statsLinesGlnIsOz)
            REP::html_li("Line " + std::to_string(s));
        
        REP::html_end_ul();
    }
}

// The file starts with BOM "EF BB BF"
void parseCSV(const std::string &filename, bool dumpHeader)
{
    if (user_map.size() > 0) {
        std::clog << "Already parsed " << filename << std::endl;
        return;
    }

    std::clog << "Reading " << filename << std::endl;
    
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

            if (columnVector.size() != 8) {
#ifdef DEBUG
                std::clog
                << "CSV Line " << statsCsvLineCount
                << ", unexpected # columns: " << columnVector.size() << std::endl;
#endif
                statsLinesWrongNumFields.push_back(statsCsvLineCount);
                
                // Try to recover as much as possible: assume column D and E are correct
                roseid_to_gln_map.insert(std::make_pair(columnVector[3], columnVector[4]));

                continue;
            }

            std::string gln_code = columnVector[4];    // E
            std::string rose_id = columnVector[3];     // D

            if (gln_code.length() == 0)
                statsLinesEmptyGlnVec.push_back(std::to_string(statsCsvLineCount));
            
            if (gln_code == "o z") {
                // We can still use it, just report it
                statsLinesGlnIsOz.push_back(statsCsvLineCount);
            }

            User user;

            if (user_map.find(rose_id) != user_map.end()) {
                user = user_map.at(rose_id);
            } else {
                user.neu_map = {
                    0, // helvepharm
                    0, // mepha
                    0, // sandoz
                    0  // spirig
                };
            }

            user.rose_id = rose_id;
            user.gln_code = gln_code;

            if (boost::contains(columnVector[5], "Helvepharm")) { // F
                user.neu_map.helvepharm = 1;
            } else if (boost::contains(columnVector[5], "Mepha")) { // F
                user.neu_map.mepha = 1;
            } else if (boost::contains(columnVector[5], "Sandoz")) { // F
                user.neu_map.sandoz = 1;
            } else if (boost::contains(columnVector[5], "Spirig")) { // F
                user.neu_map.spirig = 1;
            }

            std::vector<std::string> nameVector;
            boost::algorithm::split(nameVector, columnVector[7], boost::is_any_of(","));

            if (nameVector.size() >= 1) {
                user.name1 = nameVector[0];
            }
            if (nameVector.size() >= 2) {
                std::string city = nameVector[1];
                boost::algorithm::trim(city);
                user.city = city;
            }

            user.street = "";
            user.zip = "";
            user.special_group = "";
            
            if (user_map.find(user.gln_code) != user_map.end())
                statsGlnIsNotUniqueCount++;

            user_map[user.rose_id] = user;
            roseid_to_gln_map[user.rose_id] = user.gln_code;

        } // while

        file.close();
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
    printFileStats(filename);

#ifdef DEBUG
    std::clog
    << "Parsed " << user_map.size() << " doctors" << std::endl
    << "CSV Lines " << statsCsvLineCount
    << ", bad line(s):" << statsLinesWrongNumFields.size()
    << std::endl;
#endif
}

void parseMedixCSV(const std::string &filename)
{
    std::clog << "Reading " << filename << std::endl;

    try {
        std::ifstream file(filename);

        std::string str;
        bool header = true;
        while (std::getline(file, str))
        {
            boost::algorithm::trim_right_if(str, boost::is_any_of(" \n\r"));
            statsMedixCsvLineCount++;

            if (header) {
                header = false;
                continue;
            }

            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(CSV_SEPARATOR));

            if (columnVector.size() != 6) {
#ifdef DEBUG
                std::clog
                << "Medix CSV Line " << statsMedixCsvLineCount
                << ", unexpected # columns: " << columnVector.size() << std::endl;
#endif
                continue;
            }

            std::string rose_id = columnVector[0];    // B

            try {
                User user = user_map.at(rose_id);
                user.special_group = "medix";
                user_map.at(rose_id) = user;
            } catch (std::exception &e) {
                std::cerr
                << basename((char *)__FILE__) << ":" << __LINE__
                << " Error " << e.what()
                << std::endl;
            }
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
    << "CSV Lines " << statsMedixCsvLineCount
    << ", bad line(s):" << statsLinesWrongNumFields.size()
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
        child["special_group"] = u.second.special_group;

        tree[u.first] = child;
    }
    
    std::ofstream out(filename);
    out << tree.dump(1, '\t');          // UTF-8
    //out << tree.dump(1, '\t', true);  // ASCII with escaped UTF-8 characters, like Java
    out.close();
}

// See ShoppingCartRose.java 206 roseid_to_gln_map
void createIdsJSON(const std::string &filename)
{
    std::clog << "Writing " << filename << std::endl;

    pt::ptree tree;

    for (auto d : roseid_to_gln_map)
        tree.put(d.first, d.second);
    
    pt::write_json(filename, tree);
}

}
