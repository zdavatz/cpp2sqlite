//
//  functii.cpp
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
 * @return Loading application Header
 */

#include <cstdint>
#include <sstream>

#include "functii.h"

// In EAN13 only the last 12 digits are "directly" encoded.
// In EAN13 the first digit is encoded "indirectly" via the parity of group 1
#define N_EAN_DIGITS            12  // 8 for EAN8

#define EAN_GROUP_1_DIGITS      6   // 4 for EAN8
#define EAN_GROUP_2_DIGITS      6   // 4 for EAN8

#define LINES_PER_DIGIT         7       // each digit is seven lines apart
#define LINES_SEPARATORS        (3+5+3) // lead middle trail
#define N_LINES                 (N_EAN_DIGITS * LINES_PER_DIGIT + LINES_SEPARATORS)

// SVG rendering constants
#define QUIET_ZONE_WIDTH        5  // should be 9
#define LABEL_HEIGHT            20
#define LINES_Y_TOP             0
#define LINE_Y_BOT_LINES_SHORT  30
#define LINE_Y_BOT_LINES_LONG   45
#define CODE_Y                  44
#define SVG_LINE_WIDTH          2
#define SVG_HEIGHT              50
#define SVG_QUIET_ZONE_WIDTH    (QUIET_ZONE_WIDTH * SVG_LINE_WIDTH)
#define SVG_WIDTH               (N_LINES*SVG_LINE_WIDTH + SVG_QUIET_ZONE_WIDTH)

namespace EAN13
{
    static uint8_t b[N_LINES];   // vertical lines in the image

    // To indirectly encode the first digit
    static bool parities[10][6] = {
        {0, 0, 0, 0, 0, 0}, {0, 0, 1, 0, 1, 1}, {0, 0, 1, 1, 0, 1}, {0, 0, 1, 1, 1, 0}, {0, 1, 0, 0, 1, 1},
        {0, 1, 1, 0, 0, 1}, {0, 1, 1, 1, 0, 0}, {0, 1, 0, 1, 0, 1}, {0, 1, 0, 1, 1, 0}, {0, 1, 1, 0, 1, 0}
    };

/**
 * @author: Stefan Halus
 * @version: 1.0
 * @return Completes the 9-digit string in the code with the previous missing digits, replaced by 7
 * @param codDat[] User-entered Code
 */
std::string appendChecksum(const char countryCode[], const char codDat[])
{
    int n = strlen(codDat);
    int i = 0;
    char sirSapte[9] = "";
    
	while (codDat[i] >= '0' && codDat[i] <= '9')
		i++;

    if (i != n)
		std::cerr << "Error code: " << codDat << std::endl;

    // Patch missing digits with 7. Why ?
    if (n < 9) {
		int j;
		for (j = 0; j < 9 - n; j++)
            sirSapte[j] = '7';

        sirSapte[j] = '\0';
	}

    char ean13[13];
    strcpy(ean13, countryCode);
	strcat(ean13, sirSapte);
	strcat(ean13, codDat);

    int checkSum = calculateChecksum(ean13);
	ean13[12] = checkSum + '0';

    return ean13;
}

/**
 * @author: Stefan Halus
 * @version: 1.0
 * @return Generates the EAN13 checksum digit
 * @param  ean12 is the string value of the code entered
 */
int calculateChecksum(const std::string ean13)
{
    int evens = 0;
    int odds = 0;

    for (int i = 0; i < 12; i = i + 2) {
		evens += ean13[i] - '0';
		odds  += ean13[i + 1] - '0';
	}

    int S = evens + 3 * odds;
	int checkSum = 10 - S % 10;
    if (checkSum == 10)
        checkSum = 0;
    
	return checkSum;
}

/**
 * Generates the bit string required for the bar code, group G (even)
 * @param valoare is the value or figure to convert
 * @param b[] Matrix with written bits
 * @param poz The starting bit of the attribution
 */
void G(const int valoare,
       int &poz)
{
	switch ((valoare - '0')) {
	case 0: // 0100111
		b[poz + 1] = b[poz + 4] = b[poz + 5] = b[poz + 6] = 1;
		break;
	case 1: // 0110011
		b[poz + 1] = b[poz + 2] = b[poz + 5] = b[poz + 6] = 1;
		break;
	case 2: // 0011011
		b[poz + 2] = b[poz + 3] = b[poz + 5] = b[poz + 6] = 1;
		break;
	case 3: // 0100001
		b[poz + 1] = b[poz + 6] = 1;
		break;
	case 4: // 0011101
		b[poz + 2] = b[poz + 3] = b[poz + 4] = b[poz + 6] = 1;
		break;
	case 5: // 0111001
		b[poz + 1] = b[poz + 2] = b[poz + 3] = b[poz + 6] = 1;
		break;
	case 6: // 0000101
		b[poz + 4] = b[poz + 6] = 1;
		break;
	case 7: // 0010001
		b[poz + 2] = b[poz + 6] = 1;
		break;
	case 8: // 0001001
		b[poz + 3] = b[poz + 6] = 1;
		break;
	case 9: // 0010111
		b[poz + 2] = b[poz + 4] = b[poz + 5] = b[poz + 6] = 1;
		break;
	}
    
    poz += LINES_PER_DIGIT;
}

/**
 * Generates the bit string required for the bar code, group L (odd)
 * @param Value is the value or figure to convert
 * @param b[] Matrix with written bits
 * @param poz The starting bit of the attribution
 */
void L(const int valoare,
       int &poz)
{
	switch ((valoare - '0')) {
	case 0: // 0001101
		b[poz + 3] = b[poz + 4] = b[poz + 6] = 1;
		break;
	case 1: // 0011001
		b[poz + 2] = b[poz + 3] = b[poz + 6] = 1;
		break;
	case 2: // 0010011
		b[poz + 2] = b[poz + 5] = b[poz + 6] = 1;
		break;
	case 3: // 0111101
		b[poz + 1] = b[poz + 2] = b[poz + 3] = b[poz + 4] = b[poz + 6] = 1;
		break;
	case 4: // 0100011
		b[poz + 1] = b[poz + 5] = b[poz + 6] = 1;
		break;
	case 5: // 0110001
		b[poz + 1] = b[poz + 2] = b[poz + 6] = 1;
		break;
	case 6: // 0101111
		b[poz + 1] = b[poz + 3] = b[poz + 4] = b[poz + 5] = b[poz + 6] = 1;
		break;
	case 7: // 0111011
		b[poz + 1] = b[poz + 2] = b[poz + 3] = b[poz + 5] = b[poz + 6] = 1;
		break;
	case 8: // 0110111
		b[poz + 1] = b[poz + 2] = b[poz + 4] = b[poz + 5] = b[poz + 6] = 1;
		break;
	case 9: // 0001011
		b[poz + 3] = b[poz + 5] = b[poz + 6] = 1;
		break;
	}
    
    poz += LINES_PER_DIGIT;
}

/**
 * Generates the bit string required for the bar code, pattern R
 * @param valoare este valoarea sau cifra de convertit
 * @param b[] matricea cu biții scriși
 * @param poz bitul de start de la care se face atribuirea
 */
void R(const int valoare,
       int &poz)
{
	switch ((valoare - '0')) {
	case 0: //		1110010
		b[poz] = b[poz + 1] = b[poz + 2] = b[poz + 5] = 1;
		break;
	case 1: //		1100110
		b[poz] = b[poz + 1] = b[poz + 4] = b[poz + 5] = 1;
		break;
	case 2: //		1101100
		b[poz] = b[poz + 1] = b[poz + 3] = b[poz + 4] = 1;
		break;
	case 3: //		1000010
		b[poz] = b[poz + 5] = 1;
		break;
	case 4: //		1011100
		b[poz] = b[poz + 2] = b[poz + 3] = b[poz + 4] = 1;
		break;
	case 5: //		1001110
		b[poz] = b[poz + 3] = b[poz + 4] = b[poz + 5] = 1;
		break;
	case 6: //		1010000
		b[poz] = b[poz + 2] = 1;
		break;
	case 7: //		1000100
		b[poz] = b[poz + 4] = 1;
		break;
	case 8: //		1001000
		b[poz] = b[poz + 3] = 1;
		break;
	case 9: //		1110100
		b[poz] = b[poz + 1] = b[poz + 2] = b[poz + 4] = 1;
		break;
	}

    poz += LINES_PER_DIGIT;
}

/**
 * @author: Stefan Halus
 * @version: 1.0
 * @return Generates HTML + SVG code that represents the barcode and related explanatory texts
 * @param nean13 represents the final bar code, including the amount of control
 */
std::string createSvg(const std::string &productName,
                      const std::string &productCode)
{
    int linesTop = LINES_Y_TOP;
    int lineBotShort = LINE_Y_BOT_LINES_SHORT;
    int lineBotLong = LINE_Y_BOT_LINES_LONG;
    int svgHeight = SVG_HEIGHT;
    int codeY = CODE_Y;
    
    if (!productName.empty()) {
        linesTop += LABEL_HEIGHT;
        lineBotShort += LABEL_HEIGHT;
        lineBotLong += LABEL_HEIGHT;
        svgHeight += LABEL_HEIGHT;
        codeY += LABEL_HEIGHT;
    }

    for (int i = 0; i < N_LINES; i++)
        b[i] = 0;

    uint8_t start = productCode[0] - '0';
    int idx = 0; // line index for b[]

    // lead
    b[idx++] = 1;
	b[idx++] = 0;
    b[idx++] = 1;

    for (int i = 0; i < EAN_GROUP_1_DIGITS; i++)
        if (parities[start][i])
            G(productCode[1 + i], idx);
        else
            L(productCode[1 + i], idx);

    // separator
    b[idx++] = 0;
    b[idx++] = 1;
    b[idx++] = 0;
    b[idx++] = 1;
    b[idx++] = 0;

    for (int i = 0; i < EAN_GROUP_2_DIGITS; i++)
		R(productCode[7 + i], idx);

    // trail
    b[idx++] = 1;
    b[idx++] = 0;
    b[idx++] = 1;

    std::ostringstream cod;
    cod << "<svg height=\"" << svgHeight << "\" width=\"" << SVG_WIDTH << "\">" << std::endl;
    cod << "<rect height=\"100%\" width=\"100%\" fill=\"white\" />" << std::endl;

    if (!productName.empty()) {
        cod << "<text x=\"104\" y=\"16\" letter-spacing=\"2\" text-anchor=\"middle\">"
			<< productName << "</text>" << std::endl;
    }
    
    cod << "<g style=\"stroke:black; stroke-width:" << SVG_LINE_WIDTH << "\">" << std::endl;

    int pozx = SVG_QUIET_ZONE_WIDTH, y2;
	for (int i = 0; i < N_LINES; i++) {
		if (b[i] == 1) {
			if (i == 0 || i == 2 || i == 46 || i == 48 || i == 92 || i == 94) {
				y2 = lineBotLong;
			}
            else {
				y2 = lineBotShort;
			}

            cod << "<line x1=\"" << pozx
                << "\" y1=\"" << linesTop
                << "\" x2=\"" << pozx
                << "\" y2=\"" << y2 << "\" />" << std::endl;
		}
        
		pozx += SVG_LINE_WIDTH;
	}

    cod << "</g>" << std::endl;

    // Show numeric value of code under the bars
    cod << "<text x=\"0\" y=\"" << codeY << "\">" << productCode[0] << "</text>" << std::endl;
	cod << "<text x=\"21\" y=\"" << codeY << "\" letter-spacing=\"5\">" << productCode.substr(1,6) << "</text>" << std::endl;
	cod << "<text x=\"113\" y=\"" << codeY << "\" letter-spacing=\"5\">" << productCode.substr(7,6) << "</text>" << std::endl;

    cod << "</svg>";

    return cod.str();
}

}
