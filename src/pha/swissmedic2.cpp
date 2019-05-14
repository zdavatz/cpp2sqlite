//
//  swissmedic2.cpp
//  pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 10 May 2019
//

// Tgere is no GTIN. We match it to swissmedic 1 via column A

#include <iostream>

#include <xlnt/xlnt.hpp>

#include "swissmedic2.hpp"
#include "gtin.hpp"

#define COLUMN_A        0   // GTIN (5 digits)
#define COLUMN_B        1   // dosage number
#define COLUMN_E        4   // Type d'autorisation
#define COLUMN_I        8   // ATC

#define FIRST_DATA_ROW_INDEX    7

namespace SWISSMEDIC2
{
    std::vector<pharmaExtraRow> pharmaExtraVec;
    
void parseXLXS(const std::string &filename)
{
    xlnt::workbook wb;
    wb.load(filename);
    auto ws = wb.active_sheet();
    
    std::clog << std::endl << "Reading swissmedic extended XLSX" << std::endl;
    
    unsigned int skipHeaderCount = 0;
    for (auto row : ws.rows(false)) {
        ++skipHeaderCount;
        if (skipHeaderCount <= FIRST_DATA_ROW_INDEX) {
#if 0 //def DEBUG_PHARMA
            if (skipHeaderCount == FIRST_DATA_ROW_INDEX) {
                int i=0;
                for (auto cell : row) {
                    xlnt::column_t::index_t col_idx = cell.column_index();
                    std::clog << i++
                    << "\t" << xlnt::column_t::column_string_from_index(col_idx)
                    << "\t<" << cell.to_string() << ">" << std::endl;
                }
            }
#endif
            continue;
        }
        
        std::vector<std::string> aSingleRow;
        for (auto cell : row) {
            //std::clog << cell.to_string() << std::endl;
            aSingleRow.push_back(cell.to_string());
        }
        
        pharmaExtraRow pxr;
        pxr.rn5 = GTIN::padToLength(5, aSingleRow[COLUMN_A]);
        pxr.dosageNr = aSingleRow[COLUMN_B];
        pxr.authType = aSingleRow[COLUMN_E];
        
        pharmaExtraVec.push_back(pxr);
        
#ifdef DEBUG
        static int k=0;
        if (k++ < 55) {
            std::clog
            << "rn5: " << rn5
            << ", authType: " << authType
            << std::endl;
        }
#endif
    }
}
    
std::string getAuthorizationByAtc(const std::string &atc, const std::string &dn)
{
    std::string auth;
    
    for (auto px : pharmaExtraVec) {
        if ((px.rn5 == atc) && (px.dosageNr == dn)){
            auth = px.authType;
            break;
        }
    }
    
    return auth;
}

}
