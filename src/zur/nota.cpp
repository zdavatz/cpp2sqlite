//
//  nota.cpp
//  zurrose
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 1 Nov 2019
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <libgen.h>     // for basename()

#include "nota.hpp"

namespace pt = boost::property_tree;

namespace NOTA
{
constexpr std::string_view CSV_SEPARATOR = ";";

struct notaPosition {
    std::string pharma_code;
    std::string quantity;
    std::string status;
    std::string delivery_date;
    std::string last_order_date;
};

struct notaDoctor {
    std::string id;
    std::vector<notaPosition> notaVec;
};

std::vector<notaDoctor> doctorVec;

// Parse-phase stats
unsigned int statsNotaNumLines = 0;

void parseCSV(const std::string &filename)
{
    std::clog << std::endl << "Reading " << filename << std::endl;

    try {
        std::ifstream file(filename);
        
        std::string str;

        while (std::getline(file, str))
        {
            boost::algorithm::trim_right_if(str, boost::is_any_of("\n\r"));
            statsNotaNumLines++;

            // No header
            
            std::vector<std::string> columnVector;
            boost::algorithm::split(columnVector, str, boost::is_any_of(CSV_SEPARATOR));

            // Variable number of columns, but in multiples of 5, plus 1

            auto n = columnVector.size();
            if ((n - 1) % 5 != 0) {
                std::clog << "Unexpected # columns: " << columnVector.size() << std::endl;
                exit(EXIT_FAILURE);
            }

            notaDoctor doctor;
            doctor.id = columnVector[0];
            doctor.notaVec.clear();

            for (int i=1; i<=(n-5); i+= 5) {
                notaPosition np;
                np.pharma_code = columnVector[i];
                np.quantity = columnVector[i+1];
                np.status = columnVector[i+2];
                np.delivery_date = columnVector[i+3]; // could be empty
                np.last_order_date = columnVector[i+4];
                doctor.notaVec.push_back(np);
            }

            // TODO:: stats update max num of items per doctor from doctor.notaVec.size()

            doctorVec.push_back(doctor);
        }
    }
    catch (std::exception &e) {
        std::cerr
        << basename((char *)__FILE__) << ":" << __LINE__
        << " Error " << e.what()
        << std::endl;
    }
    
#ifdef DEBUG
    std::clog
    << "Parsed " << statsNotaNumLines << " lines"
    << "\n\t" << doctorVec.size() << " doctors"
    << std::endl;
#endif
}

void createJSON(const std::string &filename)
{
    std::clog << "Writing " << filename << std::endl;

    pt::ptree tree;

    for (auto d : doctorVec) {

        pt::ptree children;

        for (auto n : d.notaVec) {

            pt::ptree child;

            child.put("delivery_date", n.delivery_date);
            child.put("last_order_date", n.last_order_date);
            child.put("pharma_code", n.pharma_code);
            
            // FIXME: boost seems to treat everything as strings
            // possible solution is https://github.com/nlohmann/json#json-as-first-class-data-type
            child.put("quantity", std::stoi(n.quantity));

            child.put("status", n.status);
            children.push_back(std::make_pair("", child));
        }

        tree.add_child(d.id, children);
    }

    pt::write_json(filename, tree);
}
}
