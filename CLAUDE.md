# CLAUDE.md

## Project Overview
cpp2sqlite is a C++ tool that generates SQLite databases containing Swiss Healthcare Public Domain Drug Information. It aggregates data from multiple Swiss health authority sources (Swissmedic, BAG, Refdata, etc.) into SQLite databases used by AmiKo and other applications.

## Build
```bash
cd scripts && source steps_public1.source && ./download.sh
source steps_public2.source && ./download.sh
cd ../build && cmake .. && make -j9
```

## Key Executables
- `cpp2sqlite` - Main tool, generates amiko_db SQLite databases
- `pharma` - Generates pharma.csv
- `sai` - Processes SAI (Refdata structured article information)
- `zurrose` - Processes Zur Rose data

All support `--fhir` flag to use BAG FHIR ndjson instead of BAG XML.

## FHIR NDJSON Parsing
- Source: `src/bagFHIR.cpp` / `src/bagFHIR.hpp`
- Input: `downloads/fhir-sl.ndjson` from https://epl.bag.admin.ch
- Each line is a FHIR Bundle with entries: MedicinalProductDefinition, RegulatedAuthorization, PackagedProductDefinition, Ingredient
- Prices (EFP/PP) and reimbursement data are in RegulatedAuthorization extensions under `reimbursementSL` > `productPrice`
- Legal status codes map to categories: A (756005022001), B (756005022003), C (756005022005), D (756005022007, 756005022008), E (756005022009)

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
