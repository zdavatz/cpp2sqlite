//
//  aips.hpp
//  cpp2sqlite
//
//  Created by Alex Bettarini on 16 Jan 2019
//

#ifndef aips_hpp
#define aips_hpp

#include "medicine.h"

namespace AIPS
{
    
MedicineList & parseXML(const std::string &filename);

}

#endif /* aips_hpp */
