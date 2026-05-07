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

    std::vector<BAG::Preparation> prepList;
    PackageMap packMap;

    // Parse-phase stats
    unsigned int statsPackCount = 0;
    std::vector<std::string> statsGtinEmptyVec;

    // Usage stats
    unsigned int statsTotalGtinCount = 0;

    enum PriceType {
        PriceTypeUnknown = 0,
        PriceTypeRetail = 1,
        PriceTypeExFactory = 2
    };

static
void printFileStats(const std::string &filename)
{
    REP::html_h2("BAG FHIR");
    //REP::html_p(std::string(basename((char *)filename.c_str())));
    REP::html_p(filename);

    REP::html_start_ul();
    REP::html_li("preparations: " + std::to_string(prepList.size()));
    REP::html_li("packs: " + std::to_string(statsPackCount));
    REP::html_li("packs without GTIN: " + std::to_string(statsGtinEmptyVec.size()));
    REP::html_end_ul();

    if (statsGtinEmptyVec.size() > 0) {
        REP::html_h3("GTIN empty");
        REP::html_start_ol();
        for (auto s : statsGtinEmptyVec)
            REP::html_li(s);

        REP::html_end_ol();
    }
}

void printUsageStats()
{
    REP::html_h2("BAG FHIR");

    REP::html_start_ul();
    REP::html_li("GTINs used: " + std::to_string(statsTotalGtinCount));
    REP::html_end_ul();
}

void parseNDJSON(const std::string &filename,
                 const std::string &language,
                 bool verbose)
{
    std::ifstream file(filename);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            nlohmann::json lineJson = nlohmann::json::parse(line);
            BAG::Preparation p = jsonToPreparation(lineJson, language);
            prepList.push_back(p);
        }
        file.close();
    }
    printFileStats(filename);
}

// Trailing ".NN" suffix of a ClinicalUseDefinition id (e.g. "CYRAMZA.01" → "01").
// Returns empty when the id has no trailing all-digit segment.
static std::string nnSuffix(const std::string &id) {
    auto pos = id.rfind('.');
    if (pos == std::string::npos || pos + 1 >= id.size()) return "";
    std::string tail = id.substr(pos + 1);
    for (char c : tail) {
        if (c < '0' || c > '9') return "";
    }
    return tail;
}

// Extract the indication text from a ClinicalUseDefinition.  The BAG feed
// ships `indication` as a single object; the FHIR spec also allows an array.
// Path:
//   indication.diseaseSymptomProcedure.concept.text
// fallback: indication.extension[url == "limitationText"].valueString
static std::string cudIndicationText(const nlohmann::json &resource) {
    if (!resource.contains("indication")) return "";
    const auto &indNode = resource["indication"];
    const nlohmann::json *ind = nullptr;
    if (indNode.is_object()) {
        ind = &indNode;
    } else if (indNode.is_array() && !indNode.empty()) {
        ind = &indNode[0];
    }
    if (!ind) return "";

    if (ind->contains("diseaseSymptomProcedure") &&
        (*ind)["diseaseSymptomProcedure"].contains("concept") &&
        (*ind)["diseaseSymptomProcedure"]["concept"].contains("text") &&
        (*ind)["diseaseSymptomProcedure"]["concept"]["text"].is_string()) {
        return (*ind)["diseaseSymptomProcedure"]["concept"]["text"].get<std::string>();
    }
    if (ind->contains("extension") && (*ind)["extension"].is_array()) {
        for (const auto &e : (*ind)["extension"]) {
            if (e.value("url", "") == "limitationText" && e.contains("valueString") &&
                e["valueString"].is_string()) {
                return e["valueString"].get<std::string>();
            }
        }
    }
    return "";
}

// Walk the bundle and collect the bundle-scoped FOPHDossierNumber + each
// indication ClinicalUseDefinition.  Build the XXXXX.NN codes per the
// rust2xml convention.  Empty when no SL price-model data is present.
static std::vector<BAG::IndicationCode> collectBundleIndC(const nlohmann::json &json) {
    std::vector<BAG::IndicationCode> out;
    if (!json.contains("entry") || !json["entry"].is_array()) return out;

    std::string dossier;
    // (cudId, nn, text), preserving bundle order
    std::vector<std::tuple<std::string, std::string, std::string>> cuds;

    for (const auto &entry : json["entry"]) {
        if (!entry.contains("resource")) continue;
        const auto &res = entry["resource"];
        const std::string rt = res.value("resourceType", "");

        if (rt == "RegulatedAuthorization" && dossier.empty() &&
            res.contains("extension") && res["extension"].is_array()) {
            for (const auto &ext : res["extension"]) {
                std::string url = ext.value("url", "");
                if (url.size() < 16 || url.substr(url.size() - 16) != "/reimbursementSL") {
                    continue;
                }
                if (!ext.contains("extension") || !ext["extension"].is_array()) continue;
                for (const auto &sub : ext["extension"]) {
                    if (sub.value("url", "") != "FOPHDossierNumber") continue;
                    if (sub.contains("valueIdentifier") &&
                        sub["valueIdentifier"].contains("value") &&
                        sub["valueIdentifier"]["value"].is_string()) {
                        dossier = sub["valueIdentifier"]["value"].get<std::string>();
                    }
                }
                if (!dossier.empty()) break;
            }
        } else if (rt == "ClinicalUseDefinition") {
            // Skip non-"indication" CUDs (contraindication, interaction, etc).
            // The BAG feed ships `type` as a plain string; the FHIR spec also
            // permits a CodeableConcept (`type.text`).
            std::string typeText;
            if (res.contains("type")) {
                const auto &t = res["type"];
                if (t.is_string()) {
                    typeText = t.get<std::string>();
                } else if (t.is_object() && t.contains("text") && t["text"].is_string()) {
                    typeText = t["text"].get<std::string>();
                }
            }
            if (typeText != "indication") continue;
            if (!res.contains("id") || !res["id"].is_string()) continue;
            std::string id = res["id"].get<std::string>();
            std::string nn = nnSuffix(id);
            if (nn.empty()) continue;
            cuds.emplace_back(id, nn, cudIndicationText(res));
        }
    }

    if (dossier.empty() || cuds.empty()) return out;

    for (const auto &[cudId, nn, text] : cuds) {
        BAG::IndicationCode ic;
        ic.code = dossier + "." + nn;
        ic.cudId = cudId;
        ic.text = text;
        out.push_back(std::move(ic));
    }
    return out;
}

BAG::Preparation jsonToPreparation(nlohmann::json json, const std::string &language) {
    BAG::Preparation preparation;
    // Bundle-scoped Indikationscodes (BAG XXXXX.NN), applied to every pack.
    preparation.indicationCodes = collectBundleIndC(json);
    std::string entryID = json["id"];
    for (nlohmann::json entry : json["entry"]) {
        std::string resourceType = entry["resource"]["resourceType"];
        if (resourceType == "MedicinalProductDefinition") {
            auto atcCode = entry["resource"]["classification"][0]["coding"][0]["code"];
            if (atcCode.type() != nlohmann::json::value_t::string) {
                continue;
            }
            std::string atcCodeStr = atcCode.get<std::string>();
            preparation.atcCode = atcCodeStr;

            for (nlohmann::json name : entry["resource"]["name"]) {
                std::string languageCode = name["usage"][0]["language"]["coding"][0]["code"];
                std::string productName = name["productName"];
                if (languageCode.find(language) == 0) {
                    preparation.name = productName;
                }
            }

            for (nlohmann::json classification : entry["resource"]["classification"]) {
                nlohmann::json code = classification["coding"][0]["code"];
                if (code.type() == nlohmann::json::value_t::string) {
                    std::string codeStr = code.get<std::string>();
                    // https://fhir.ch/ig/ch-epl/CodeSystem-ch-epl-foph-product-type.html
                    if (codeStr == "756001003001") {
                        // Generic product
                        preparation.orgen = "G";
                    } else if (codeStr == "756001003002") {
                        // Originator product
                        preparation.orgen = "O";
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
                preparation.swissmedNo = regnrStr;
            }
        } else if (resourceType == "PackagedProductDefinition") {
            BAG::Pack pack;
            nlohmann::json gtin = entry["resource"]["packaging"]["identifier"][0]["value"];
            if (gtin.type() == nlohmann::json::value_t::string) {
                std::string gtinStr = gtin.get<std::string>();
                pack.gtin = gtinStr;
            }
            GTIN::verifyGtin13Checksum(pack.gtin);
            pack.description = entry["resource"]["description"];

            nlohmann::json legalStatusCodeJson = entry["resource"]["legalStatusOfSupply"][0]["code"]["coding"][0]["code"];
            if (legalStatusCodeJson.type() == nlohmann::json::value_t::string) {
                std::string legalStatusCode = legalStatusCodeJson.get<std::string>();
                if (legalStatusCode == "756005022001") {
                    pack.category = "A";
                } else if (legalStatusCode == "756005022003") {
                    pack.category = "B";
                } else if (legalStatusCode == "756005022005") {
                    pack.category = "C";
                } else if (legalStatusCode == "756005022007" || legalStatusCode == "756005022008") {
                    pack.category = "D";
                } else if (legalStatusCode == "756005022009") {
                    pack.category = "E";
                }
            }

            std::string resourceId = entry["resource"]["id"];
            // Fill the prices
            for (nlohmann::json subEntry : json["entry"]) {
                if (subEntry["resource"]["resourceType"] == "RegulatedAuthorization" &&
                    subEntry["resource"]["subject"][0]["reference"] == "CHIDMPPackagedProductDefinition/" + resourceId)
                {
                    // std::clog << " subEntry: " << subEntry["resource"]["extension"].dump() << std::endl;

                    nlohmann::json name = subEntry["resource"]["contained"][0]["name"];
                    if (name.type() == nlohmann::json::value_t::string) {
                        pack.partnerDescription = name.get<std::string>();
                    }

                    for (nlohmann::json reimbursementSL : subEntry["resource"]["extension"]) {
                        if (reimbursementSL["url"] == "http://fhir.ch/ig/ch-epl/StructureDefinition/reimbursementSL") {
                            for (nlohmann::json reimbursementSLExtension : reimbursementSL["extension"]) {
                                if (reimbursementSLExtension["url"] == "http://fhir.ch/ig/ch-epl/StructureDefinition/productPrice") {
                                    PriceType type = PriceTypeUnknown;
                                    double moneyValue;
                                    std::string moneyCurrency;
                                    std::string changeDate;
                                    for (nlohmann::json productPriceExtension : reimbursementSLExtension["extension"]) {
                                        std::string productPriceExtensionUrl = productPriceExtension["url"];
                                        if (productPriceExtensionUrl == "type") {
                                            std::string code = productPriceExtension["valueCodeableConcept"]["coding"][0]["code"];
                                            // https://build.fhir.org/ig/bag-epl/bag-epl-fhir/branches/__default/ValueSet-ch-epl-foph-pricetype.html
                                            if (code == "756002005001") {
                                                // retail price
                                                type = PriceTypeRetail;
                                            } else if (code == "756002005002") {
                                                // Ex-factory price
                                                type = PriceTypeExFactory;
                                            }
                                        } else if (productPriceExtensionUrl == "value") {
                                            moneyValue = productPriceExtension["valueMoney"]["value"];
                                            moneyCurrency = productPriceExtension["valueMoney"]["currency"];
                                        } else if (productPriceExtensionUrl == "changeDate") {
                                            changeDate = productPriceExtension["valueDate"];
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
                                } else if (reimbursementSLExtension["url"] == "costShare") {
                                    int valueInteger = reimbursementSLExtension["valueInteger"];
                                    preparation.sb = valueInteger;
                                }
                            }
                        }
                    }
                }
            }
            // Inherit the bundle-scoped Indikationscodes (BAG XXXXX.NN).
            pack.indicationCodes = preparation.indicationCodes;

            statsPackCount++;
            if (!pack.gtin.empty()) {
                preparation.packs.push_back(pack);
            } else {
                statsGtinEmptyVec.push_back(pack.description + ", " + preparation.swissmedNo);
            }
        }
    }
    return preparation;
}

int getAdditionalNames(const std::string &rn,
                       std::set<std::string> &gtinUsed,
                       GTIN::oneFachinfoPackages &packages)
{
    std::set<std::string>::iterator it;
    int countAdded = 0;

    for (BAG::Preparation preparation : prepList) {
        if (rn != preparation.swissmedNo)
            continue;

        for (BAG::Pack p : preparation.packs) {
            std::string g13 = p.gtin;
            std::string paf = getPricesAndFlags(g13, "", p.category);

            // Build GTIN if missing
            it = gtinUsed.find(g13);
            if (it == gtinUsed.end()) { // not found in list of used GTINs, we must add the name
                countAdded++;
                statsTotalGtinCount++;

                std::string onePackageInfo;
#ifdef DEBUG_IDENTIFY_NAMES
                onePackageInfo += "bagfhir+";
#endif
                if (!p.description.empty()) {
                    onePackageInfo += p.description;
                } else {
                    onePackageInfo += preparation.name;
                }

                if (!paf.empty())
                    onePackageInfo += paf;

                gtinUsed.insert(g13);
                packages.gtin.push_back(g13);
                packages.name.push_back(onePackageInfo);
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

    for (BAG::Preparation preparation : prepList)
        for (BAG::Pack p : preparation.packs)
            if (gtin == p.gtin) {
                BAG::packageFields pf;

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

                if (preparation.sb != 0) {
                    flagsVector.push_back("SB " + std::to_string(preparation.sb) + "%");
                }

                if (!preparation.orgen.empty())
                    flagsVector.push_back(preparation.orgen);

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

void getIndCByRegnr(const std::string &regnr,
                    std::string &codes,
                    std::string &text)
{
    codes.clear();
    text.clear();
    if (regnr.empty()) return;

    // The same Swissmedic regnr can appear across multiple FHIR bundles
    // (often only one of them carries the ClinicalUseDefinitions). Walk the
    // entire prepList and merge IndicationCodes from every matching prep,
    // dedup by code; that way we don't lose IndCs because the first matching
    // prep happens to have an empty indicationCodes list.
    std::set<std::string> seen;
    std::vector<std::string> codeOut;
    std::vector<std::string> textOut;
    for (const BAG::Preparation &prep : prepList) {
        if (prep.swissmedNo != regnr) continue;
        for (const BAG::IndicationCode &ic : prep.indicationCodes) {
            if (ic.code.empty()) continue;
            if (!seen.insert(ic.code).second) continue;
            codeOut.push_back(ic.code);
            if (!ic.text.empty()) {
                textOut.push_back(ic.code + ": " + ic.text);
            }
        }
    }
    codes = boost::algorithm::join(codeOut, ",");
    text = boost::algorithm::join(textOut, "\n");
}

std::vector<std::string> getGtinList() {
    std::vector<std::string> list;

    for (BAG::Preparation preparation : prepList)
        for (BAG::Pack p : preparation.packs)
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

BAG::packageFields getPackageFieldsByGtin(const std::string &gtin)
{
    return packMap[gtin];
}

std::vector<std::string> gtinWhichDoesntStartWith7680()
{
    std::vector<std::string> result;
    for (BAG::Preparation prep : prepList) {
        for (BAG::Pack pack : prep.packs) {
            std::string gtin = pack.gtin;
            if (gtin.substr(0,4) != "7680") {
                result.push_back(gtin);
            }
        }
    }
    return result;
}

bool getPreparationAndPackageByGtin(const std::string &gtin, BAG::Preparation *outPrep, BAG::Pack *outPack) {
    for (BAG::Preparation prep : prepList) {
        for (BAG::Pack pack : prep.packs) {
            if (pack.gtin == gtin) {
                *outPrep = prep;
                *outPack = pack;
                return true;
            }
        }
    }
    return false;
}

PreparationList getPrepList()
{
    return prepList;
}

}
