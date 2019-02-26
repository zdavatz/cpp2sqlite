//
//  report.hpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 25 Feb 2019
//

#ifndef logger_hpp
#define logger_hpp

#include <iostream>
#include <fstream>

namespace REP
{
    void init(std::string logDir,
              std::string filename,
              bool verbose=false);
    void terminate();

    void html_h1(const std::string &msg);
    void html_h2(const std::string &msg);
    void html_h3(const std::string &msg);
    void html_h4(const std::string &msg);
    void html_p(const std::string &msg);
    void html_div(const std::string &msg);
    void html_start_ul();
    void html_end_ul();
    void html_li(const std::string &msg);
    
//    // http://www.drdobbs.com/cpp/logging-in-c/201804215
//    class Log
//    {
//    public:
//        Log();
//        virtual ~Log();
//    protected:
//        std::ofstream os;
//    };
}

#endif /* logger_hpp */
