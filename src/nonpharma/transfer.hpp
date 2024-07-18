//
//  transfer.hpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by b123400 on 9 Nov 2023
//

#ifndef transfer_hpp
#define transfer_hpp

namespace TRANSFER
{

struct Entry
{
public:
    std::string pharma_code;
    std::string ean13;
    std::string description;
    double price;
    double pub_price;
};

std::map<std::string, TRANSFER::Entry> getEntries();

void parseDAT(const std::string &filename);
Entry getEntryWithGtin(const std::string &gtin);

}

#endif /* transfer_hpp */
