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

    std::clog << std::endl << "Reading swissmedic XLSX" << std::endl;

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

// Note that multiple rows can have the same gtin_5
// Each row corresponds to a different package (=gtin_13)
std::string getNames(const std::string &rn)
{
    std::string names;
    int i=0;
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        std::string gtin_5 = theWholeSpreadSheet.at(rowInt).at(COLUMN_A);
        // TODO: to speed up do a numerical comparison so that we can return when gtin5>rn
        // assuming that column A is sorted
        if (gtin_5 == rn) {
            if (i>0)
                names += "\n";

            //names += "swm-";
            names += theWholeSpreadSheet.at(rowInt).at(COLUMN_C);
            //std::cerr << " FOUND at: " << rowInt << ", name " << name << std::endl;
            i++;
        }
    }
    
#if 0
    if (i>1)
        std::cout << basename((char *)__FILE__) << ":" << __LINE__ << " rn: " << rn << " FOUND " << i << " times" << std::endl;
#endif
    
    return names;
}

int countRowsWithRn(const std::string &rn)
{
    int count = 0;
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        std::string gtin5 = theWholeSpreadSheet.at(rowInt).at(COLUMN_A);
        // TODO: to speed up do a numerical comparison so that we can return when gtin5>rn
        // assuming that column A is sorted
        if (gtin5 == rn)
            count++;
    }

    return count;
}

// https://www.gs1.org/services/how-calculate-check-digit-manually
// See Utilities.java, line 41, function getChecksum
int getChecksum(std::string eanStr)
{
    std::cout << "";
    int val=0;
#if 0
    // TODO:
    for (int i=0; i<eanStr.length(); i++) {
        val += (Integer.parseInt(eanStr.charAt(i)+""))*((i%2==0)?1:3);
    }
#endif
    
    int checksum_digit = 10 - (val % 10);
    if (checksum_digit == 10)
        checksum_digit = 0;
    
    return checksum_digit;
}
    
bool findGtin(const std::string &gtin)
{
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        std::string regnr = theWholeSpreadSheet.at(rowInt).at(COLUMN_A);
        std::string code = theWholeSpreadSheet.at(rowInt).at(COLUMN_K);
        std::string gtin_13 = "7680" + regnr + code;
#if 0
        // TODO: use checksum
#else
        if (gtin == gtin.substr(0,12)) // pos, len
            return true;
#endif
    }

    return false;
}

}
