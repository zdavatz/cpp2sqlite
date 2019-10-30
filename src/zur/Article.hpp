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
    std::string availability;   // D
    std::string therapy_code;   // G
    std::string galen_form;
    std::string pack_unit;
    std::string regnr;
    std::string atc_code;
    std::string atc_class;
    std::string rose_base_price;
    std::string rose_supplier;
    std::string replace_ean_code;
    std::string replace_pharma_code;
    std::string flags;
    std::string public_price;
    std::string exfactory_price;
    std::string pack_title_FR;
    std::string galen_code;

    int pack_size; // F
    int stock;
    int likes;

    bool off_the_market;
    bool npl_article;
    bool dlk_flag;
};

#endif /* Article_hpp */
