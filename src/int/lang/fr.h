//
//  fr.h
//  interaction
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 12 March 2019
//

#ifndef fr_h
#define fr_h

const std::string nameSection1_fr = "Classe de risque";      // Risk class
const std::string nameSection2_fr = "Möglicher Effekt";      // Possible effect
const std::string nameSection3_fr = "Mechanismus";           // Mechanism
const std::string nameSection4_fr = "Empfohlene Massnahmen"; // Recommended measures

const std::string legendA_fr = "Keine Massnahmen notwendig";      // No measures necessary
const std::string legendB_fr = "Vorsichtsmassnahmen empfohlen";   // precautions recommended
const std::string legendC_fr = "Regelmässige Überwachung";        // Regular monitoring
const std::string legendD_fr = "Kombination vermeiden";           // Avoid combination
const std::string legendX_fr = "Kontraindiziert";                 // Contraindicated

void stringsFromFr()
{
    nameSection1 = nameSection1_fr;
    nameSection2 = nameSection2_fr;
    nameSection3 = nameSection3_fr;
    nameSection4 = nameSection4_fr;
    
    legendA = legendA_fr;
    legendB = legendB_fr;
    legendC = legendC_fr;
    legendD = legendD_fr;
    legendX = legendX_fr;
}

#endif /* fr_h */
