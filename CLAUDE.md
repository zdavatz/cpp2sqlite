# CLAUDE.md

## Project Overview
cpp2sqlite is a C++ tool that generates SQLite databases containing Swiss Healthcare Public Domain Drug Information. It aggregates data from multiple Swiss health authority sources (Swissmedic, BAG, Refdata, etc.) into SQLite databases used by AmiKo and other applications.

## Build
```bash
cd scripts && source steps_public1.source && ./download.sh
source steps_public2.source && ./download.sh
cd ../build && cmake .. && make -j9
```

Note: `BOOST_BIND_GLOBAL_PLACEHOLDERS` is defined in CMakeLists.txt to suppress Boost bind placeholder deprecation warnings (the project doesn't use boost::bind directly, but Boost headers trigger the warning internally).

## Key Executables
- `cpp2sqlite` - Main tool, generates amiko_db SQLite databases
- `pharma` - Generates pharma.csv
- `sai` - Processes SAI (Refdata structured article information)
- `zurrose` - Processes Zur Rose data

All support `--fhir` flag to use BAG FHIR ndjson instead of BAG XML.

## FHIR NDJSON Parsing
- Source: `src/bagFHIR.cpp` / `src/bagFHIR.hpp`
- Input: `downloads/fhir-sl-{de,fr,it}.ndjson` from https://epl.bag.admin.ch/static/fhir/foph-sl-export-latest-{de,fr,it}.ndjson (per-language NDJSON)
- Each line is a FHIR Bundle with entries: MedicinalProductDefinition, RegulatedAuthorization, PackagedProductDefinition, Ingredient, ClinicalUseDefinition
- Prices (EFP/PP) and reimbursement data are in RegulatedAuthorization extensions under `reimbursementSL` > `productPrice`
- Legal status codes map to categories: A (756005022001), B (756005022003), C (756005022005), D (756005022007, 756005022008), E (756005022009)
- When `--fhir` is active, all price lookups (including from Refdata and Swissmedic) must use `BAGFHIR::getPricesAndFlags()` instead of `BAG::getPricesAndFlags()`, since `BAG::parseXML()` is not called and its prepList is empty

## Indikationscode (BAG XXXXX.NN)
- Mandatory on SL prescriptions and invoices from 2026-07-01 (BAG Rundschreiben 2026-02-19); insurers may reject non-compliant invoices from 2027-01-01
- Source: `src/bagFHIR.cpp` `collectBundleIndC()` — bundle-scoped pass over each NDJSON line (one Bundle per line)
- `XXXXX` comes from one `RegulatedAuthorization.extension[reimbursementSL].extension[FOPHDossierNumber].valueIdentifier.value`; `.NN` is the trailing all-digit segment of each `ClinicalUseDefinition.id` (e.g. `CYRAMZA.01`); only `type == "indication"` CUDs are picked up. The actual feed ships `type` as a plain string and `indication` as a single object — both shapes are handled.
- Limitations text comes from `indication.diseaseSymptomProcedure.concept.text`, with fallback `indication.extension[url == "limitationText"].valueString`
- Stored on `BAG::Preparation.indicationCodes` (and copied onto every `Pack`) as `IndicationCode {code, cudId, text}`
- `BAGFHIR::getIndCByRegnr(regnr, &codes, &text)` returns the joined columns the schema expects: comma-joined codes + newline-joined `code: text` lines, deduped by code, in bundle order. Mirrors rust2xml ≥ 3.1.12 (`INDIKATIONSCODE` / `INDIKATIONSCODE_TEXT` XML elements). The function walks the entire `prepList` and merges IndicationCodes from every prep that shares the queried Swissmedic regnr, deduped by code — required because the same 5-digit Swissmedic regnr typically spans multiple FHIR bundles (only one of which carries the type=indication ClinicalUseDefinitions). An earlier early-return on the first matching prep with empty indicationCodes silently dropped IndC for ~80% of regnrs.
- Schema/binding: `Sql::useIndC` is set from `flagFHIR` in `openDB()`. When true, the `amikodb` schema gains two trailing TEXT columns (`indikationscode`, `indikationscode_text`) and `insertRow` binds positions 19/20. Non-FHIR builds emit the legacy 19-column schema, so apps reading by column index keep working.
- iOS consumer: `generikacc/Generika/IndCSection.swift` (issue zdavatz/generikacc#102) — guarded by `AmikoDBManager isIndcColumnAvailable` so older DB snapshots don't break the app

## Zur Rose Artikelstamm (Exfact column)
- Feed: `artikel_vollstamm_zurrose.csv` and `artikel_stamm_zurrose.csv` are downloaded via `scripts/download_zr.sh` (SFTP from ftp.zur-rose.ch).
- Schema change (2026-05): a trailing column `Exfact` (V, index 21) was appended, growing the row from 21 to 22 semicolon-separated fields. All four column-count guards must match the new width — `src/zur/stamm.cpp:69`, `src/zur/stamm.cpp:129`, `src/zur/voll.cpp:152`, `src/zur/voll.cpp:355`. Note that `voll.cpp::parseCSV` (line 152) silently `continue`s on mismatch instead of exiting, so a missed bump there empties the output DB without an error message.
- `Exfact` is the Zur Rose ex-factory price and is wired into `rosedb.exfprice` as a fallback for the BAG EFP in `voll.cpp::parseCSV` (~line 304). BAG values stay canonical for SL-listed drugs; Zur Rose Exfact fills the long tail. In `--fhir` builds without BAG XML this lifts `rosedb.exfprice` population from 0/163858 to 163858/163858.

## zurrose SQLite lifecycle (do not double-close)
`VOLL::closeDB()` calls `sqlite3_finalize(statement)` then `sqlite3_close(db)`, so it must be called exactly once. Calling it twice double-frees both handles and corrupts the glibc heap — the failure surfaces as `free(): invalid next size (fast)` on process exit and, depending on arena state, may not reproduce on smaller workloads (atcdb hid it while fulldb crashed every run). `src/zur/main.cpp` previously had a stray second `closeDB` block right after the fulldb/atcdb branch; the single call inside the branch is the correct lifecycle.

## ean13 is a separate git submodule
`src/c2s/ean13` is a git submodule pointing at `ywesee/BarcodeGenerator` (separate org from `zdavatz/cpp2sqlite`). Fixes there require: commit + push inside the submodule on master, then bump the parent's submodule pointer in a parent commit. Consumers (and the build server) must run `git submodule update` after pulling the parent.

## GCC 13+ header transitivity
GCC 13+ (and current libstdc++) no longer pulls many standard headers transitively. New code that uses standard types/streams must `#include` them explicitly or the Funtoo build server breaks:
- `<cstdint>` for `uint8_t` / `int32_t` / etc. (tripped `src/c2s/ean13/functii.cpp` — fixed in ywesee/BarcodeGenerator c1f17da)
- `<fstream>` for `std::ofstream` / `std::ifstream` (tripped `src/sap/main.cpp`, `src/dru/main.cpp` — fixed in 0203368, plus `bagFHIR.cpp`, `c2s/peddose.cpp`, `c2s/sappinfo.cpp`, `c2s/refdata.cpp` cleaned up preemptively)

If a new source file uses these (or `<sstream>`, `<iomanip>`, etc.) without an explicit include, it will compile locally on older toolchains and break on the build server.

## Directory Structure
- `src/` - C++ source files
- `src/c2s/` - cpp2sqlite specific sources
- `src/pha/` - pharma specific sources
- `src/sai/` - SAI specific sources
- `src/zur/` - zurrose specific sources
- `scripts/` - Build and download scripts
- `input/` - Input data files (not in git)
- `downloads/` - Downloaded source files (not in git)
- `output/` - Generated databases and reports (not in git)
