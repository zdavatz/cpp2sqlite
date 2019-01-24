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
#define COLUMN_S       18   // application field

#define FIRST_DATA_ROW_INDEX    5

namespace SWISSMEDIC
{
    
    std::vector< std::vector<std::string> > theWholeSpreadSheet;
    std::vector<std::string> regnrs;        // padded to 5 characters (digits)
    std::vector<std::string> packingCode;   // padded to 3 characters (digits)

void parseXLXS(const std::string &filename)
{
    xlnt::workbook wb;
    wb.load(filename);
    auto ws = wb.active_sheet();

    std::clog << std::endl << "Reading swissmedic XLSX" << std::endl;

    int skipHeaderCount = 0;
    for (auto row : ws.rows(false)) {
        if (++skipHeaderCount <= FIRST_DATA_ROW_INDEX)
            continue;

        std::vector<std::string> aSingleRow;
        for (auto cell : row) {
            //std::clog << cell.to_string() << std::endl;
            aSingleRow.push_back(cell.to_string());
        }

        theWholeSpreadSheet.push_back(aSingleRow);

        // Precalculate padded regnr
        std::string rn5 = aSingleRow[COLUMN_A];
        while (rn5.length() < 5) // pad with zeros
            rn5 = "0" + rn5;

        regnrs.push_back(rn5);

        // Precalculate padded packing code
        std::string code3 = aSingleRow[COLUMN_K];
        while (code3.length() < 3) // pad with zeros
            code3 = "0" + code3;
        
        packingCode.push_back(code3);
    }

    std::clog << "swissmedic rows: " << theWholeSpreadSheet.size() << std::endl;
}

// Note that multiple rows can have the same value in column A
// Each row corresponds to a different package (=gtin_13)
std::string getNames(const std::string &rn)
{
    std::string names;
    int i=0;
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        std::string rn5 = regnrs[rowInt];

        // TODO: to speed up do a numerical comparison so that we can return when gtin5>rn
        // assuming that column A is sorted
        if (rn5 == rn) {
            if (i>0)
                names += "\n";

            //names += "swm-";
            names += theWholeSpreadSheet.at(rowInt).at(COLUMN_C);
            //std::clog << " FOUND at: " << rowInt << ", name " << name << std::endl;
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
        std::string gtin_5 = regnrs[rowInt];
        // TODO: to speed up do a numerical comparison so that we can return when gtin5>rn
        // assuming that column A is sorted
        if (gtin_5 == rn)
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
        std::string rn5 = regnrs[rowInt];
        std::string code3 = packingCode[rowInt];
        std::string gtin13 = "7680" + rn5 + code3;
#if 0
        // TODO: use checksum
#else
        if (gtin13 == gtin.substr(0,12)) // pos, len
            return true;
#endif
    }

    return false;
}

std::string getApplication(const std::string &rn)
{
    std::string app;
    for (int rowInt = 0; rowInt < theWholeSpreadSheet.size(); rowInt++) {
        if (rn == regnrs[rowInt]) {
            app = theWholeSpreadSheet.at(rowInt).at(COLUMN_S) + " (Swissmedic)";
            break;
        }
    }

    return app;
}
}
