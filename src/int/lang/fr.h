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
const std::string nameSection2_fr = "Effet possible";      // Possible effect
const std::string nameSection3_fr = "Mécanisme";           // Mechanism
const std::string nameSection4_fr = "Mesures recommandées"; // Recommended measures

const std::string legendA_fr = "Aucune mesure n'est nécessaire";      // No measures necessary
const std::string legendB_fr = "Précautions recommandées";   // precautions recommended
const std::string legendC_fr = "Suivi régulier";        // Regular monitoring
const std::string legendD_fr = "Éviter la combinaison";           // Avoid combination
const std::string legendX_fr = "Contre-indiqué";                 // Contraindicated

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
