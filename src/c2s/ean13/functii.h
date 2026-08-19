//
//  functii.h
//  BarcodeGenerator
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Modified by Alex Bettarini on 7 Feb 2019
//

/**
 * Created on: 3 dec. 2016
 * @author: Stefan Halus
 * @version: 1.0
 * @return Încară prototipul funcțiilor fonosite pentru efectuarea activităților programului.
 */

#ifndef FUNCTII_H_
#define FUNCTII_H_

#include <iostream>
#include <cstring>

namespace EAN13
{
    std::string appendChecksum(const char[], const char[]);
    int calculateChecksum(const std::string);

    std::string createSvg(const std::string &,
                          const std::string &);
}

#endif
/* FUNCTII_H_ */
