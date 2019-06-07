//
//  ddd.cpp
//  pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 21 May 2019
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <libgen.h>     // for basename()

#include <boost/algorithm/string_regex.hpp>

#include "ddd.hpp"

//namespace po = boost::program_options;

namespace DDD
{
    std::vector<dddLine> lineVec;

void parseCSV(const std::string &filename)
{
    std::clog << std::endl << "Reading ddd CSV" << std::endl;

    try {
        //std::clog << std::endl << "Reading CSV" << std::endl;
        std::ifstream file(filename);

        std::string str;
        while (std::getline(file, str)) {
            std::string firstColumn;
            std::string thirdColumn;
#if 0
            // It doesn't work because sometimes column 3 is in double quotes with multiple additional ';'
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(";"));
            
            if (columnVector.size() != 3) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }

            firstColumn = columnVector[0];
            thirdColumn = columnVector[2];
#else
            std::string::size_type pos1 = str.find(";");
            firstColumn = str.substr(0, pos1); // pos, len
            auto col23 = str.substr(pos1+1);
            
            std::string::size_type pos2 = col23.find(";");
            //auto col2 = col23.substr(0, pos2);  // pos, len

            // Take whatever comes after the second semicolon
            thirdColumn = col23.substr(pos2+1); // pos, len
#endif

            boost::algorithm::trim(thirdColumn);
            boost::replace_all(thirdColumn, ",", "."); // "0,5" --> "0.5"
//            boost::algorithm::trim_left_if(thirdColumn, boost::is_any_of("\""));
//            boost::algorithm::trim_right_if(thirdColumn, boost::is_any_of("\""));
            boost::algorithm::trim_if(thirdColumn, boost::is_any_of("\""));

            std::regex rgx(R"((\d+(\.\d+)?)\s(mg|g)\s(O))");  // tested at https://regex101.com
            std::smatch match;
            if (std::regex_search(thirdColumn, match, rgx)) {
#ifdef DEBUG_DDD
                std::clog
                << "thirdColumn <" << thirdColumn << ">"
                << ", match.size <" << match.size() << ">"
                << std::endl;
                for (auto m : match) {
                    std::clog
                    << "\t <" << m << ">"
                    << std::endl;
                }
#endif
                if ((match.size() == 5) && (match[4] == "O")){
                    dddLine ln;
                    ln.atc = firstColumn;
                    
                    std::string as = match[1];
                    double a = std::atof(as.c_str());
                    ln.dailyDosage_mg = a;
                    
                    if (match[3] == "g")
                        ln.dailyDosage_mg *= 1000;
                    
                    lineVec.push_back(ln);
                }
            }
        }
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
#ifdef DEBUG_DDD
    std::clog
    << "lineVec <" << lineVec.size() << ">" // 1724
    << std::endl;
#endif
}
    
bool getDailyDosage_mg_byATC(const std::string &atc,
                             double *ddd_mg)
{    
    for (auto lv : lineVec)
        if (lv.atc == atc) {
            *ddd_mg = lv.dailyDosage_mg;
            return true;
        }
    
    return false;
}

}

