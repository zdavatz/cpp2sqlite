//
//  bag.cpp
//  cpp2sqlite, pharma
//
//  ©ywesee GmbH -- all rights reserved
//  License GPLv3.0 -- see License File
//  Created by Alex Bettarini on 23 Jan 2019
//

#include <set>
#include <iomanip>
#include <sstream>
#include <libgen.h>     // for basename()
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "bagFHIR.hpp"
#include "gtin.hpp"
//#include "swissmedic.hpp"
#include "report.hpp"

namespace pt = boost::property_tree;

namespace BAGFHIR
{

    std::vector<Bundle> bundleList;
    PackageMap packMap;

    // Usage stats
    unsigned int statsTotalGtinCount = 0;
    unsigned int statsPackWithoutGtinCount = 0;

    enum PriceType {
        PriceTypeUnknown = 0,
        PriceTypeRetail = 1,
        PriceTypeExFactory = 2
    };

void parseNDJSON(const std::string &filename,
                 const std::string &language,
                 bool verbose)
{
    std::ifstream file(filename);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            nlohmann::json lineJson = nlohmann::json::parse(line);
            Bundle b = jsonToBundle(lineJson, language);
            bundleList.push_back(b);
        }
        file.close();
    }
}

Bundle jsonToBundle(nlohmann::json json, const std::string &language) {
    Bundle b;
    std::string entryID = json["id"];
    for (nlohmann::json entry : json["entry"]) {
        std::string resourceType = entry["resource"]["resourceType"];
        if (resourceType == "MedicinalProductDefinition") {
            auto atcCode = entry["resource"]["classification"][0]["coding"][0]["code"];
            if (atcCode.type() != nlohmann::json::value_t::string) {
                continue;
            }
            std::string atcCodeStr = atcCode.get<std::string>();
            b.atcCode = atcCodeStr;

            for (nlohmann::json name : entry["resource"]["name"]) {
                std::string languageCode = name["usage"][0]["language"]["coding"][0]["code"];
                std::string productName = name["productName"];
                if (languageCode.find(language) == 0) {
                    b.name = productName;
                }
            }

            for (nlohmann::json classification : entry["resource"]["classification"]) {
                nlohmann::json code = classification["coding"][0]["code"];
                if (code.type() == nlohmann::json::value_t::string) {
                    std::string codeStr = code.get<std::string>();
                    // https://fhir.ch/ig/ch-epl/CodeSystem-ch-epl-foph-product-type.html
                    if (codeStr == "756001003001") {
                        // Generic product
                        b.orgen = "G";
                    } else if (codeStr == "756001003002") {
                        // Originator product
                        b.orgen = "O";
                    }

                }
            }
        } else if (resourceType == "RegulatedAuthorization") {
            // TODO: log if more than 1
            auto regnr = entry["resource"]["identifier"][0]["value"];
            if (regnr.type() != nlohmann::json::value_t::string) {
                continue;
            }
            std::string regnrStr = regnr.get<std::string>();
            if (regnrStr.length() == 5) {
                b.regnr = regnrStr;
            }
        } else if (resourceType == "PackagedProductDefinition") {
            Pack pack;
            nlohmann::json gtin = entry["resource"]["packaging"]["identifier"][0]["value"];
            if (gtin.type() == nlohmann::json::value_t::string) {
                std::string gtinStr = gtin.get<std::string>();
                pack.gtin = gtinStr;
            }
            // TODO: GTIN::verifyGtin13Checksum(pack.gtin);
            if (pack.gtin.empty()) {
                statsPackWithoutGtinCount++;
            }
            pack.description = entry["resource"]["description"];

            std::string resourceId = entry["resource"]["id"];
            // Fill the prices
            for (nlohmann::json sub_entry : json["entry"]) {
                if (sub_entry["resource"]["resourceType"] == "RegulatedAuthorization" &&
                    sub_entry["resource"]["subject"][0]["reference"] == "PackagedProductDefinition/" + resourceId)
                {
                    // std::clog << " sub_entry: " << sub_entry["resource"]["extension"].dump() << std::endl;
                    for (nlohmann::json resourceExtension : sub_entry["resource"]["extension"]) {
                        if (resourceExtension["url"] != "http://fhir.ch/ig/ch-epl/StructureDefinition/productPrice") {
                            continue;
                        }
                        PriceType type = PriceTypeUnknown;
                        double moneyValue;
                        std::string moneyCurrency;
                        std::string changeDate;
                        for (nlohmann::json subExtension : resourceExtension["extension"]) {
                            std::string subExtensionUrl = subExtension["url"];
                            if (subExtensionUrl == "type") {
                                std::string code = subExtension["valueCodeableConcept"]["coding"][0]["code"];
                                // https://build.fhir.org/ig/bag-epl/bag-epl-fhir/branches/__default/ValueSet-ch-epl-foph-pricetype.html
                                if (code == "756002005001") {
                                    // retail price
                                    type = PriceTypeRetail;
                                } else if (code == "756002005002") {
                                    // Ex-factory price
                                    type = PriceTypeExFactory;
                                }
                            } else if (subExtensionUrl == "value") {
                                moneyValue = subExtension["valueMoney"]["value"];
                                moneyCurrency = subExtension["valueMoney"]["currency"];
                            } else if (subExtensionUrl == "changeDate") {
                                changeDate = subExtension["valueDate"];
                            }
                        }
                        if (type == PriceTypeRetail) {
                            // TODO: currency
                            pack.publicPrice = formatPriceAsMoney(moneyValue);
                        } else if (type == PriceTypeExFactory) {
                            // TODO: currency
                            pack.exFactoryPrice = formatPriceAsMoney(moneyValue);
                            // TODO: format date
                            pack.exFactoryPriceValidFrom = changeDate;
                        }
                    }
                }
            }
            if (!pack.gtin.empty()) {
                b.packs.push_back(pack);
            } else {
                std::clog << "EMPTY GTIN! " << b.regnr << "," << b.name << "pp: " << pack.publicPrice << " exp: " << pack.exFactoryPrice << std::endl;
            }
        }
    }
    return b;
}

int getAdditionalNames(const std::string &rn,
                       std::set<std::string> &gtinUsed,
                       GTIN::oneFachinfoPackages &packages)
{
    std::set<std::string>::iterator it;
    int countAdded = 0;

    for (Bundle bundle : bundleList) {
        if (rn != bundle.regnr)
            continue;

        for (Pack p : bundle.packs) {
            std::string g13 = p.gtin;
            // Build GTIN if missing
            it = gtinUsed.find(g13);
            if (it == gtinUsed.end()) { // not found in list of used GTINs, we must add the name
                countAdded++;
                statsTotalGtinCount++;

                std::string onePackageInfo;
#ifdef DEBUG_IDENTIFY_NAMES
                onePackageInfo += "bag+";
#endif
                if (!p.description.empty()) {
                    onePackageInfo += p.description;
                } else {
                    onePackageInfo += bundle.name;
                }

                std::string paf = getPricesAndFlags(g13, "", "");
                if (!paf.empty())
                    onePackageInfo += paf;

                gtinUsed.insert(g13);
                packages.gtin.push_back(g13);
                packages.name.push_back(onePackageInfo);
                std::cout << "getAdditionalNames: " << rn << " onePackageInfo:" << onePackageInfo << std::endl;
            }
        }
    }

    return countAdded;
}

// Also build a map(gtin) to be used later when writing packages column
std::string getPricesAndFlags(const std::string &gtin,
                              const std::string &fromSwissmedic,
                              const std::string &category)
{
    std::string prices;
    std::vector<std::string> flagsVector;
    bool found = false;

    for (Bundle bundle : bundleList)
        for (Pack p : bundle.packs)
            if (gtin == p.gtin) {
                packageFields pf;

                // Prices
                if (!p.exFactoryPrice.empty()) {
                    prices += "EFP " + p.exFactoryPrice;
                    pf.efp = p.exFactoryPrice;
                }

                if (!p.publicPrice.empty()) {
                    prices += ", PP " + p.publicPrice;
                    pf.pp = p.publicPrice;
                }

                if (!p.exFactoryPriceValidFrom.empty()) { // for pharma.csv
                    pf.efp_validFrom = p.exFactoryPriceValidFrom;
                }

                // Flags
                if (!category.empty())
                    flagsVector.push_back(category);

                if (!p.exFactoryPrice.empty() || !p.publicPrice.empty())
                    flagsVector.push_back("SL");  // TODO: localize to LS for French

                // if (!p.limitationPoints.empty())
                //     flagsVector.push_back("LIM" + p.limitationPoints);

                // SB: Selbstbehalt
                // https://github.com/zdavatz/cpp2sqlite/issues/236
                // if (bundle.sb == "Y")
                //     flagsVector.push_back("SB 40%");
                // else if (bundle.sb20 == "Y")
                //     flagsVector.push_back("SB 40%");
                // else if (bundle.sb20 == "N" || bundle.sb == "N")
                //     flagsVector.push_back("SB 10%");

                if (!bundle.orgen.empty())
                    flagsVector.push_back(bundle.orgen);

                pf.flags = flagsVector;
                packMap[gtin] = pf;
                found = true;
                goto prepareResult; // abort the two for loops
            } // if

prepareResult:
    // The category (input parameter) must be added even if the GTIN was not found
    if (!found) {
        if (!category.empty())
            flagsVector.push_back(category);

//        std::clog
//        << basename((char *)__FILE__) << ":" << __LINE__
//        << ", NOT FOUND: " << gtin
//        << std::endl;
    }

    std::string paf;
    if (!prices.empty())
        paf += ", " + prices;

    if (!fromSwissmedic.empty())
        paf += ", " + fromSwissmedic;

    if (flagsVector.size() > 0)
        paf += " [" + boost::algorithm::join(flagsVector, ", ") + "]";

    return paf;
}

std::vector<std::string> getGtinList() {
    std::vector<std::string> list;

    for (Bundle bundle : bundleList)
        for (Pack p : bundle.packs)
            if (!p.gtin.empty())
                list.push_back(p.gtin);

    return list;
}

std::string formatPriceAsMoney(double price)
{
    std::ostringstream s;
    s << std::fixed << std::setprecision(2) << price;
    return s.str();
}

}
