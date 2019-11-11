//
//  stamm.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 1 Nov 2019
//

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>

#include <boost/algorithm/string.hpp>

#include <libgen.h>     // for basename()

#include "stamm.hpp"

namespace STAMM
{
constexpr std::string_view CSV_SEPARATOR1 = ";";
constexpr std::string_view CSV_SEPARATOR2 = ";";

// Parse-phase stats
unsigned int statsVoigtNumValidLines = 0;
unsigned int statsVoigtEmptyAnot7 = 0;
unsigned int statsVoigtEmptyB = 0;
unsigned int statsVoigtMapAdditions = 0;
unsigned int statsVoigtMapUpdates = 0;

std::map<std::string, stockStruct> pharmaStockMap;

void parseCSV(const std::string &filename, bool dumpHeader)
{
    std::clog << std::endl << "Reading " << filename << std::endl;

    try {
        std::ifstream file(filename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str)) {
            
            if (header) {
                header = false;

                if (dumpHeader) {
                    std::ofstream outHeader(filename + ".header.txt");
                    std::vector<std::string> headerTitles;
                    boost::algorithm::split(headerTitles, str, boost::is_any_of(CSV_SEPARATOR1));
                    outHeader << "Number of columns: " << headerTitles.size() << std::endl;
                    auto colLetter = 'A';
                    for (auto t : headerTitles)
                        outHeader << colLetter++ << "\t" << t << std::endl;

                    outHeader.close();
                }

                continue;
            }
            
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(CSV_SEPARATOR1));
            
            if (columnVector.size() != 21) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }
            
            std::string pharma = columnVector[0]; // A
            if (pharma.length() > 0) {
                stockStruct stock;
                stock.zurrose = std::stoi(columnVector[8]); // I
                pharmaStockMap.insert(std::make_pair(pharma, stock));
            }
        } // while
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
#ifdef DEBUG
    std::clog << "Parsed " << pharmaStockMap.size() << " map items" << std::endl;
#endif
}

void parseVoigtCSV(const std::string &filename, bool dumpHeader)
{
    std::clog << std::endl << "Reading " << filename << std::endl;

    try {
        std::ifstream file(filename);
        
        std::string str;
        bool header = true;
        while (std::getline(file, str))
        {
            // Trim the line, otherwise when it's split in cells the B
            // column instead of being an empty cell it contains '\r'
            boost::algorithm::trim_right_if(str, boost::is_any_of("\n\r"));

            if (header) {
                header = false;

                if (dumpHeader) {
                    std::ofstream outHeader(filename + ".header.txt");
                    std::vector<std::string> headerTitles;
                    boost::algorithm::split(headerTitles, str, boost::is_any_of(CSV_SEPARATOR2));
                    outHeader << "Number of columns: " << headerTitles.size() << std::endl;
                    auto colLetter = 'A';
                    for (auto t : headerTitles)
                        outHeader << colLetter++ << "\t" << t << std::endl;

                    outHeader.close();
                }

                continue;
            }
            
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(CSV_SEPARATOR2));
            
            if (columnVector.size() != 2) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }

            statsVoigtNumValidLines++;
            std::string pharma = columnVector[0];
            int voigtStock = 0;
            if (columnVector[1].length() > 0)
                voigtStock = std::stoi(columnVector[1]);
            else
                statsVoigtEmptyB++;

            if (pharma.length() == 7)
            {
                auto search = pharmaStockMap.find(pharma);
                if (search != pharmaStockMap.end()) {
                    //stock = search->second;   // Get saved stock
                    search->second.voigt = voigtStock; // Update saved stock
                    statsVoigtMapUpdates++;
                }
                else {
                    stockStruct stock {0, voigtStock};
                    pharmaStockMap.insert(std::make_pair(pharma, stock));
                    statsVoigtMapAdditions++;
                }
            }
            else {
                statsVoigtEmptyAnot7++;
            }
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
    << "Parsed " << statsVoigtNumValidLines << " valid lines"
    << "\n\t" << statsVoigtMapUpdates+statsVoigtMapAdditions << " = "
        << statsVoigtMapUpdates << " map updates + "
        << statsVoigtMapAdditions << " map additions"
    << "\n\t" << statsVoigtEmptyAnot7 << " A column code length not 7"
    << "\n\t" << statsVoigtEmptyB << " B column empty"
    << "\n\t" << pharmaStockMap.size() << " items in stock map"
    << std::endl;
#endif
}

void createStockCSV(const std::string &filename)
{
        std::ofstream ofs;
        ofs.open(filename);
        constexpr std::string_view OUTPUT_FILE_SEPARATOR = ";";
        
        std::clog << std::endl << "Creating CSV" << std::endl;
        
        for (auto item : pharmaStockMap) {
            ofs
            << item.first << OUTPUT_FILE_SEPARATOR          // A
            << item.second.zurrose << OUTPUT_FILE_SEPARATOR // B
            << item.second.voigt << std::endl;              // C
        }
        
        ofs.close();

    #ifdef DEBUG
        std::clog << std::endl << "Created " << filename << std::endl;
    #endif
}

}
