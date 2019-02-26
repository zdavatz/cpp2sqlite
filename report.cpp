//
//  report.cpp
//  cpp2sqlite
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 25 Feb 2019
//

#define BOOST_LOG_DYN_LINK 1

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <libgen.h>     // for basename()

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "report.hpp"

namespace REP
{
    std::ofstream ofs2;
    std::vector<std::string> ul;

static
void sanitize(std::string &msg)
{
    boost::replace_all(msg, "<", "&lt;");
    boost::replace_all(msg, ">", "&gt;");
}

void log_init(std::string logDir, std::string filename)
{

    std::string fullFilename(logDir);
    fullFilename += filename;

    ofs2.open(fullFilename.c_str());
    ofs2 << "<html>";
    ofs2 << "<head>";

    ofs2 << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />";
    ofs2 << "<style>body { background-color: #dedede;} table { font-family: verdana,arial,sans-serif; font-size: 13px; color: #333333; border-width: 1px; border-color: #666666; border-collapse: collapse; width: 100% } th { border-width: 1px; padding: 8px; border-style: solid; border-color: #666666; background-color: #dedede;} td { border-width: 1px; padding: 8px; border-style: solid; border-color: #666666; background-color: #ffffff;}</style>";

    ofs2 << "</head>";
    ofs2 << "<body>" << std::endl;
}
    
void html_start_ul()
{
    ul.clear();
    ofs2 << "<ul>" << std::endl;
}

void html_end_ul()
{
    for (auto bullet : ul)
        ofs2 << bullet << std::endl;

    ofs2 << "</ul>" << std::endl;
}
    
void html_h1(const std::string &msg)
{
    ofs2 << "<hr><h1>" << msg << "</h1>" << std::endl;
}

void html_h2(const std::string &msg)
{
    ofs2 << "<h2>" << msg << "</h2>" << std::endl;
}

void html_h3(const std::string &msg)
{
    ofs2 << "<h3>" << msg << "</h3>" << std::endl;
}

void html_h4(const std::string &msg)
{
    ofs2 << "<h4>" << msg << "</h4>" << std::endl;
}

void html_p(const std::string &msg)
{
    ofs2 << "<p>" << msg << "</p>" << std::endl;
}

void html_div(const std::string &msg)
{
    std::string s(msg);
    sanitize(s);
    ofs2 << "<div>" << s << "</div>" << std::endl;
}

void html_li(const std::string &msg)
{
    std::string s(msg);
    sanitize(s);
    ul.push_back("<li>" + s + "</li>");
}

void log_msg(const std::string &msg)
{
    ofs2 << msg << std::endl;
    ofs2.flush();
}

void log_terminate()
{
    ofs2 << "</body>";
    ofs2 << "</html>";
    ofs2.close();
}

}
