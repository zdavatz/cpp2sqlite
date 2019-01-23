//
//  swissmedic.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 22 Jan 2019
//

#include <iostream>
#include <libgen.h>     // for basename()
#include <xlnt/xlnt.hpp>

#include "swissmedic.hpp"

#define COLUMN_A        0   // GTIN (5 digits)
#define COLUMN_C        2   // name
#define COLUMN_K       10   // packaging code (3 digits)
#define COLUMN_N       13   // category (A..E)

namespace SWISSMEDIC
{
    
std::vector< std::vector<std::string> > theWholeSpreadSheet;

void parseXLXS(const std::string &filename)
{
    xlnt::workbook wb;
    wb.load(filename);
    auto ws = wb.active_sheet();

    std::clog << "Reading swissmedic" << std::endl;

    for (auto row : ws.rows(false)) {
        std::vector<std::string> aSingleRow;
        for (auto cell : row) {
            //std::clog << cell.to_string() << std::endl;
            aSingleRow.push_back(cell.to_string());
        }

        theWholeSpreadSheet.push_back(aSingleRow);
    }

    std::clog << "swissmedic rows: " << theWholeSpreadSheet.size() << std::endl;
}

std::string getName(const std::string &rn)
{
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        std::string gtin = theWholeSpreadSheet.at(rowInt).at(COLUMN_A);
        if (gtin == rn) {
            std::string name = theWholeSpreadSheet.at(rowInt).at(COLUMN_C);
            //std::cerr << basename((char *)__FILE__) << ":" << __LINE__  << " FOUND at: " << rowInt << ", name " << name << std::endl;
            return name;
        }
    }
    
    return "";
}
}
