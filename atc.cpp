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
//#include <boost/locale.hpp>
//#include <boost/algorithm/string/case_conv.hpp>

#include "atc.hpp"
#include "swissmedic.hpp"

namespace ATC
{
    std::map<std::string, std::string> atcMap;
    std::string statsFilename;
    std::set<std::string> atcMissingSet;

void parseTXT(const std::string &filename,
              const std::string &language,
              bool verbose)
{
    statsFilename = filename;
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
        << " Error " << e.what()
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
        //boost::algorithm::trim(trimmed);  // TODO: improve the regular expression so we don't need to trim
        boost::erase_all(trimmed, " ");   // "D08A C52" --> "D08AC52"
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

// The input string is a single atc
std::string getTextByAtc(const std::string atc)
{
    std::string text;
    auto search = atcMap.find(atc);
    if (search != atcMap.end())
        text = search->second;
    
    return text;
}

// The input string is in the format "atccode[,atccode]*"
std::string getTextByAtcs(const std::string atcs)
{
    std::string text;
    std::string firstAtc = getFirstAtc(atcs);

    return getTextByAtc(firstAtc);
}
    
static
std::string getTextByAtc(const std::string atc, const int n)
{
    std::string s;

    if (atc.length() > n) {
        std::string sub = atc.substr(0,n);
        s = getTextByAtc(sub);

        if (s.empty()) {
            // Report missing
            std::set<std::string>::iterator it;
            it = atcMissingSet.find(sub);
            if (it == atcMissingSet.end()) { // Report it only once by using a set
#if 0
                std::cerr
                << basename((char *)__FILE__) << ":" << __LINE__
                << " ### Error ATC <" << atc << ">"
                << ", missing branch <" << sub << ">"
                << std::endl;
#endif

                atcMissingSet.insert(sub);
            }
        }
    }

    // Change casing to match what the Java code does
    //s = boost::locale::to_title(s);
    //s = boost::to_lower_copy<std::string>(s);
    //s[0] = std::toupper(s[0]);

    return s;
}

// The input string is in the format "atccode[,atccode]*;text"
std::string getClassByAtcColumn(const std::string atcColumn)
{
    auto atc = getFirstAtcInAtcColumn(atcColumn);

    std::string s1 = getTextByAtc(atc, 1);
    std::string s3 = getTextByAtc(atc, 3);
    std::string s4 = getTextByAtc(atc, 4);
    std::string s5 = getTextByAtc(atc, 5);

    return s1 + ";" + s3 + ";" + s4 + "#" + s5 + "#";
}

std::string getFirstAtcInAtcColumn(const std::string atcColumn)
{
    std::string::size_type len = atcColumn.find(";");
    auto atcs = atcColumn.substr(0, len); // pos, len
    return getFirstAtc(atcs);
}

// The input parameter 'atcs' could be a list of comma separated ATCs
// Return the first one
std::string getFirstAtc(const std::string atcs)
{
    std::vector<std::string> atcVector;
    boost::algorithm::split(atcVector, atcs, boost::is_any_of(","), boost::token_compress_on);

    return atcVector[0];
}

void printStats()
{
    std::cout
    << "ATC: " << atcMissingSet.size()
    << " missing branches from " << statsFilename
    << std::endl;
    
    std::cerr << boost::algorithm::join(atcMissingSet, ", ") << std::endl;
}

}
