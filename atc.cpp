//
//  atc.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 30 Jan 2019
//

#include <iostream>
#include <vector>
#include <string>
#include <libgen.h>     // for basename()
#include <boost/algorithm/string.hpp>

#include "atc.hpp"
#include "swissmedic.hpp"

namespace ATC
{
    int statsRecoveredAtcCount = 0;

void validate(const std::string &regnrs, std::string &atc)
{
    std::string inputAtc = atc;  // save the original string for debugging

    // Trim leading and trailing spaces
    // Do this first because in one case we get just a space " " as the input atc
    boost::algorithm::trim(atc);

    if ((atc == "na") ||
        (atc == "???") ||
        (atc == "-"))
    {
        // Make it empty so we can search for it in swissmedic
        atc = "";
    }

    // If empty get it from swissmedic column G, based on the regnrs (just the first regnr)
    if (atc.empty()) {
        std::vector<std::string> rnVector;
        boost::algorithm::split(rnVector, regnrs, boost::is_any_of(", "), boost::token_compress_on);
        atc = SWISSMEDIC::getAtcFromFirstRn(rnVector[0]);
#if 1
        std::clog
        << basename((char *)__FILE__) << ":" << __LINE__
        << ", regnrs: <" << regnrs << ">"
        << ", rn[0]: " << rnVector[0]
        << ", input ATC <" << inputAtc << ">";
        if (atc.empty())
            std::clog << ", remains empty";
        else
            std::clog << ", recovered: " << atc;

        std::clog << std::endl;
#endif
        if (!atc.empty())
            statsRecoveredAtcCount++;

        return;
    }

    // TODO: cleanup HTML stuff &nbsp; &acute;

    // TODO: add ";" and localized text from 'atc_codes_multi_lingual.txt'
}

}
