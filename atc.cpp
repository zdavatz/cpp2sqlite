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
#include <regex>
#include <boost/algorithm/string.hpp>

#include "atc.hpp"
#include "swissmedic.hpp"

namespace ATC
{
    int statsRecoveredAtcCount = 0;

// This is required for ATC originating from aips, because they have lots of
// extra stuff in the string
void validate(const std::string &regnrs, std::string &atc)
{
    std::string inputAtc = atc;  // save the original string for debugging
    std::vector<std::string> rnVector;
    boost::algorithm::split(rnVector, regnrs, boost::is_any_of(", "), boost::token_compress_on);

    // Trim leading and trailing spaces
    // Do this first because in one case we get just a space " " as the input atc
    boost::algorithm::trim(atc);

//    if ((atc == "na") ||
//        (atc == "???") ||
//        (atc == "-"))
//    {
//        // Make it empty so we can search for it in swissmedic
//        atc = "";
//    }

#if 0
    // If empty get it from swissmedic column G, based on the regnrs (just the first regnr)
    if (atc.empty()) {
        atc = SWISSMEDIC::getAtcFromFirstRn(rnVector[0]);
        if (!atc.empty())
            statsRecoveredAtcCount++;

        return;
    }
#endif

    // Cleanup. First extract a list of ATCs from each input atc string
    // see also RealExpertInfo.java:922
    std::regex r(R"([A-Z]\d{2}[A-Z]\s?[A-Z]?\s?(\d{2})?)"); // tested at https://regex101.com
    std::sregex_iterator it(atc.begin(), atc.end(), r);
    std::sregex_iterator it_end;
    std::vector<std::string> atcVector;
    while (it != it_end) {
        atcVector.push_back(it->str());
        ++it;
    }
    
#if 0
    if (atcVector.size() == 0) {  // If empty get it from swissmed
        atc = SWISSMEDIC::getAtcFromFirstRn(rnVector[0]);
        if (!atc.empty())
            statsRecoveredAtcCount++;
    }
    else
#endif
    {
        int atcCount = 0;
        std::string outputAtc;
        for (auto s : atcVector) {
            if (atcCount++ > 0)
                outputAtc += ","; // separator
            
            outputAtc += s;
        }

        atc = outputAtc;
    }

    // TODO: add ";" and localized text from 'atc_codes_multi_lingual.txt'
}

}
