//
//  de.h
//  interaction
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 12 March 2019

#ifndef de_h
#define de_h

const std::string nameSection1_de = "Risikoklasse";           // Risk class
const std::string nameSection2_de = "Möglicher Effekt";       // Possible effect
const std::string nameSection3_de = "Mechanismus";            // Mechanism
const std::string nameSection4_de = "Empfohlene Massnahmen";  // Recommended measures

const std::string legendA_de = "Keine Massnahmen notwendig";      // No measures necessary
const std::string legendB_de = "Vorsichtsmassnahmen empfohlen";   // precautions recommended
const std::string legendC_de = "Regelmässige Überwachung";        // Regular monitoring
const std::string legendD_de = "Kombination vermeiden";           // Avoid combination
const std::string legendX_de = "Kontraindiziert";                 // Contraindicated

void stringsFromDe()
{
    nameSection1 = nameSection1_de;
    nameSection2 = nameSection2_de;
    nameSection3 = nameSection3_de;
    nameSection4 = nameSection4_de;
    
    legendA = legendA_de;
    legendB = legendB_de;
    legendC = legendC_de;
    legendD = legendD_de;
    legendX = legendX_de;
}

#endif /* de_h */
