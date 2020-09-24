//
//  Article.hpp
//  zurrose
//
//  Created by Alex Bettarini on 30 Oct 2019.
//

#ifndef Article_hpp
#define Article_hpp

#include <string>

struct Article
{
public:
    std::string pharma_code;    // A
    std::string pack_title;     // B

    std::string ean_code;       // C
    std::string flags;
    std::string regnr;

    std::string availability;   // D
    std::string therapy_code;   // G

    std::string atc_code;       // H
    std::string atc_class;

    std::string rose_supplier;  // J

    std::string galen_form {};  // K
    std::string galen_code;

    std::string pack_unit;      // L
    std::string rose_base_price;    // M
    std::string replace_ean_code;   // N
    std::string replace_pharma_code;// O
 
    std::string public_price;       // R
    std::string exfactory_price;

    std::string dispose_flag;       // S
    std::string pack_title_FR;      // U

    int pack_size;  // F
    int stock;      // I Lagerbestand = stock, a number > 0 means in stock

    int likes;

    bool off_the_market;    // P
    bool npl_article;       // Q
    bool dlk_flag;          // T
};

#endif /* Article_hpp */
