//
//  gtin.hpp
//  cpp2sqlite
//
//  Created by Alex Bettarini on 24/01/2019.
//

#ifndef gtin_hpp
#define gtin_hpp

namespace GTIN
{
    
char getGtin13Checksum(std::string gtin12); // TODO: move to its own gtin class
bool verifyGtin13Checksum(std::string gtin13); // TODO: move to its own gtin class

}

#endif /* gtin_hpp */
