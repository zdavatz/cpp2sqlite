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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <libgen.h>     // for basename()

#include "kunden.hpp"
#include "report.hpp"

namespace pt = boost::property_tree;

namespace KUNDEN
{
constexpr std::string_view CSV_SEPARATOR = ";";

struct pharmaGroup {
    float helvepharm;
    float mepha;
    float sandoz;
    float spirig;
};

struct User {
    std::string rose_id;    // A
    std::string gln_code;   // D  Also key element of the map, note it could be "o z" or even ""
    std::string name1;      // O  Company name 1
    std::string street;     // R
    std::string zip;        // T
    std::string city;       // U
    std::string email;      // V
    std::string special_rebate; // AC
    bool top_customer {false};  // AD Topkunde FLAG (zur Rose)
    float revenue {0.0f};    // AE Jahres Umsatz = annual turnover

    // Takeda Pharma (not needed)
    // The Java code sets some of the following from 'downloads/gln_codes_people_xlsx.xlsx'

    std::string addr_type;      // TODO: // S: shipping, B: billing, O: Office
    bool bet_mittel {false};
    std::string bm_type;        // TODO:
    std::string capabilities;   // TODO:
    std::string category;       // TODO: // arzt, spital, drogerie, ...
    std::string country;        // TODO:
    std::string fax;            // TODO:
    std::string first_name;     // TODO:
    std::string ideale_id;      // TODO:
    bool is_human {true};
    std::string last_name;      // TODO:
    std::string name2;          // TODO: // company name 2
    std::string name3;          // TODO: // company name 3
    std::string number;         // TODO:
    std::string owner;          // TODO: // owner[0]=i -> IBSA, owner[1]=d -> Desitin, etc...
    std::string phone;          // TODO:
    std::string sap_id;         // TODO:
    bool selbst_disp  {false};
    std::string specialities;   // TODO:
    std::string status {"A"};   // TODO: // Default: Aktiv
    std::string title;          // TODO:
    std::string xpris_id;       // TODO:
    
    pharmaGroup dlk_map;
    pharmaGroup expenses_map;
    pharmaGroup rebate_map;
};

std::map<std::string, User> user_map;
std::map<std::string, std::string> roseid_to_gln_map;

// Parse-phase stats
unsigned int statsCsvNumLines = 0;
std::vector<int> statsLinesWrongNumFields;
std::vector<std::string> statsLinesEmptyGlnVec; // strings so that they can be comveniently joined
std::vector<int> statsLinesGlnIsOz;

static
float getFloatOrZero(const std::string &cell)
{
    if (cell.length() > 0)
        return std::stof(cell);

    return 0.0;
}

static
void printFileStats(const std::string &filename)
{
    REP::html_h2("KUNDEN");
    //REP::html_p(std::string(basename((char *)filename.c_str())));
    REP::html_p(filename);
    
    if (statsLinesWrongNumFields.size() > 0) {
        REP::html_p("Number of lines with wrong number of fields (skipped): " + std::to_string(statsLinesWrongNumFields.size()));
        REP::html_start_ul();
        for (auto s : statsLinesWrongNumFields)
            REP::html_li("Line " + std::to_string(s));
        
        REP::html_end_ul();
    }

    if (statsLinesEmptyGlnVec.size() > 0) {
        REP::html_p("Lines with empty 'GLN' (column D). Note that 'Knr Rose' (column A) will be used instead in 'rose_conditions.json'. Total count: " + std::to_string(statsLinesEmptyGlnVec.size()));

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

void parseCSV(const std::string &filename)
{
    std::clog << std::endl << "Reading " << filename << std::endl;

    try {
        std::ifstream file(filename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str))
        {
            boost::algorithm::trim_right_if(str, boost::is_any_of(" \n\r"));
            statsCsvNumLines++;

            if (header) {
                header = false;
#ifdef DEBUG
                std::vector<std::string> headerTitles;
                boost::algorithm::split(headerTitles, str, boost::is_any_of(CSV_SEPARATOR));
                std::clog << "Number of columns: " << headerTitles.size() << std::endl;
                auto colLetter1 = ' ';
                auto colLetter2 = 'A';
                int index = 0;
                for (auto t : headerTitles) {
                    std::clog << index++ << ") " << colLetter1 << colLetter2 << "\t" << t << std::endl;
                    if (colLetter2++ == 'Z') {
                        colLetter2 = 'A';
                        if (colLetter1 == ' ')
                            colLetter1 = 'A';
                        else
                            colLetter1++;
                    }
                }
#endif
                continue;
            }
            
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(CSV_SEPARATOR));
            
            if (columnVector.size() != 31) {
                std::clog
                << "CSV Line " << statsCsvNumLines
                << ", unexpected # columns: " << columnVector.size() << std::endl;

                statsLinesWrongNumFields.push_back(statsCsvNumLines);
                
                // Try to recover as much as possible: assume column A and D are correct
                roseid_to_gln_map.insert(std::make_pair(columnVector[0], columnVector[3]));

                continue;
            }
            
            std::string gln_code = columnVector[3];    // D
            if (gln_code.length() == 0) {
                statsLinesEmptyGlnVec.push_back(std::to_string(statsCsvNumLines));
            }
            
            if (gln_code == "o z") {
                // We can still use it, just report it
                statsLinesGlnIsOz.push_back(statsCsvNumLines);
            }
            
            User user;
            user.rose_id = columnVector[0];     // A
            user.gln_code = gln_code;
            user.name1 = columnVector[14];      // O
            user.street = columnVector[17];     // R
            user.zip = columnVector[19];        // T
            user.city = columnVector[20];       // U
            user.email = columnVector[21];      // V
            user.top_customer = (boost::to_lower_copy<std::string>(columnVector[29]) == "true"); // AD

            user.special_rebate = columnVector[28]; // AC
            
            if (columnVector[30].length() > 0)
                user.revenue = std::stof(columnVector[30]); // AE
            
            // TODO: See Java line 92 look up 'doctorPreferences'

            user.rebate_map = {
                getFloatOrZero(columnVector[4]),      // E
                getFloatOrZero(columnVector[5]),      // F
                getFloatOrZero(columnVector[6]),      // G
                getFloatOrZero(columnVector[7])};     // H
            
            user.expenses_map = {
                getFloatOrZero(columnVector[8]),      // I
                getFloatOrZero(columnVector[9]),      // J
                getFloatOrZero(columnVector[10]),     // K
                getFloatOrZero(columnVector[11])};    // L

            user.dlk_map = {
                getFloatOrZero(columnVector[24]),     // Y
                getFloatOrZero(columnVector[25]),     // Z
                getFloatOrZero(columnVector[26]),     // AA
                getFloatOrZero(columnVector[27])};    // AB

            // TODO:
            
            user_map.insert(std::make_pair(user.gln_code, user));
            roseid_to_gln_map.insert(std::make_pair(user.rose_id, user.gln_code));

        } // while
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
    << "Parsed " << user_map.size() << " doctors"
    << ", bad line(s):" << statsLinesWrongNumFields.size()
    << std::endl;
#endif
}

// See ShoppingCartRose.java 194 user_map
void createConditionsJSON(const std::string &filename)
{
    std::clog << "Writing " << filename << std::endl;
    pt::ptree tree;

    for (auto u : user_map)
    {
        pt::ptree child;
        child.put("addr_type", u.second.addr_type);
        child.put("bet_mittel", u.second.bet_mittel);
        child.put("bm_type", u.second.bm_type);
        child.put("capabilities", u.second.capabilities);
        child.put("category", u.second.category);
        child.put("city", u.second.city);
        child.put("country", u.second.country);

        pt::ptree dlkChild;
        dlkChild.put("helvepharm", u.second.dlk_map.helvepharm);
        dlkChild.put("mepha", u.second.dlk_map.mepha);
        dlkChild.put("sandoz", u.second.dlk_map.sandoz);
        dlkChild.put("spirig", u.second.dlk_map.spirig);
        child.add_child("dlk_map", dlkChild);

        child.put("email", u.second.email);

        pt::ptree expensesChild;
        expensesChild.put("helvepharm", u.second.expenses_map.helvepharm);
        expensesChild.put("mepha", u.second.expenses_map.mepha);
        expensesChild.put("sandoz", u.second.expenses_map.sandoz);
        expensesChild.put("spirig", u.second.expenses_map.spirig);
        child.add_child("expenses_map", expensesChild);

        child.put("fax", u.second.fax);
        child.put("first_name", u.second.first_name);
        child.put("gln_code", u.second.gln_code);
        child.put("ideale_id", u.second.ideale_id);
        child.put("is_human", u.second.is_human);
        child.put("last_name", u.second.last_name);
        child.put("name1", u.second.name1);
        child.put("name2", u.second.name2);
        child.put("name3", u.second.name3);
        child.put("number", u.second.number);
        child.put("owner", u.second.owner);
        child.put("phone", u.second.phone);

        pt::ptree rebateChild;
        rebateChild.put("helvepharm", u.second.rebate_map.helvepharm);
        rebateChild.put("mepha", u.second.rebate_map.mepha);
        rebateChild.put("sandoz", u.second.rebate_map.sandoz);
        rebateChild.put("spirig", u.second.rebate_map.spirig);
        child.add_child("rebate_map", rebateChild);

        child.put("revenue", u.second.revenue);
        child.put("sap_id", u.second.sap_id);
        child.put("selbst_disp", u.second.selbst_disp);
        child.put("special_rebate", u.second.special_rebate);
        child.put("specialities", u.second.specialities);
        child.put("status", u.second.status);
        child.put("street", u.second.street);
        child.put("title", u.second.title);
        child.put("top_customer", u.second.top_customer);
        child.put("xpris_id", u.second.xpris_id);
        child.put("zip", u.second.zip);

        if (u.first.length() > 0) {
            tree.add_child(u.first, child);
        }
        else
            tree.add_child(u.second.rose_id, child);
    }
    
    pt::write_json(filename, tree);
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
