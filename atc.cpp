//
//  atc.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 30 Jan 2019
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <libgen.h>     // for basename()
#include <regex>
#include <map>
#include <boost/algorithm/string.hpp>

#include "atc.hpp"
#include "swissmedic.hpp"

namespace ATC
{
    std::map<std::string, std::string> atcMap;
    int statsRecoveredAtcCount = 0;

void parseTXT(const std::string &filename,
              const std::string &language,
              bool verbose)
{
    try {
        std::clog << std::endl << "Reading atc TXT" << std::endl;
        std::ifstream file(filename);

        std::string str;
        while (std::getline(file, str)) {

            const std::string separator1(": ");
            std::string::size_type pos1 = str.find(separator1);
            auto atc = str.substr(0, pos1); // pos, len

            const std::string separator2("; ");
            std::string::size_type pos2 = str.find(separator2);
            auto textDe = str.substr(pos1+separator1.length(),       // pos
                                     pos2-pos1-separator1.length()); // len

            auto textFr = str.substr(pos2+separator2.length()); // pos, len

            atcMap.insert(std::make_pair(atc,
                                         language == "fr" ? textFr : textDe));
        }
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << "Error" << e.what()
        << std::endl;
    }
    
    std::clog
    << basename((char *)__FILE__) << ":" << __LINE__
    << " # lines: " << atcMap.size()
    << std::endl;
}

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

    // Cleanup. First extract a list of ATCs from each input atc string
    // see also RealExpertInfo.java:922
    std::regex regAtc(R"([A-Z]\d{2}[A-Z]\s?[A-Z]?\s?(\d{2})?)"); // tested at https://regex101.com
    std::sregex_iterator it(atc.begin(), atc.end(), regAtc);
    std::sregex_iterator it_end;
    std::vector<std::string> atcVector;
    while (it != it_end) {
        std::string trimmed = it->str();
        boost::algorithm::trim(trimmed);  // TODO: improve the regular expression so we don't need to trim
        atcVector.push_back(trimmed);
        ++it;
    }
    
    int atcCount = 0;
    std::string outputAtc;
    for (auto s : atcVector) {
        if (atcCount++ > 0)
            outputAtc += ","; // separator
        
        outputAtc += s;
    }

    atc = outputAtc;
}
    
std::string getTextFromAtc(std::string atc)
{
    std::string text;
    auto search = atcMap.find(atc);
    if (search != atcMap.end())
        text = search->second;

    return text;
}

}
