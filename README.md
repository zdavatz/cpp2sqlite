# cpp2sqlite
C++ tool to generate sqlite database containing Swiss Healthcare Public Domain Drug Information
## History
This tool is a port of [aips2sqlite](https://github.com/zdavatz/aips2sqlite)
## Requirements

- Boost
- C++17
- sqlite
- cmake
- gcc-7.3.1
- [xlnt](https://github.com/tfussell/xlnt) with `cmake -DSTATIC=on`
- jq (Command-line JSON processor)

## Installation
See [build.sh](https://github.com/zdavatz/cpp2sqlite/blob/master/scripts/build.sh)
## Usage
### cpp2sqlite
`./cpp2sqlite --inDir ~/.software/aips2sqlite/jars/downloads/`
### deepl translation
run `build/sappinfo` to generate the unique language file.
comment out the line 8 of [build.sh](https://github.com/zdavatz/cpp2sqlite/blob/master/scripts/build.sh#L8) to do the translations.
## Input Sources
_ [AIPS](http://download.swissmedicinfo.ch)\
_ [BAG XML](http://www.spezialitätenliste.ch/File.axd?file=XMLPublications.zip)\
_ [Refdata](https://www.refdata.ch/content/page_1.aspx?Nid=6&Aid=628&ID=291)\
_ [Swissmedic](https://www.swissmedic.ch/dam/swissmedic/de/dokumente/listen/excel-version_zugelasseneverpackungen.xlsx.download.xlsx/excel-version_zugelasseneverpackungen.xlsx)\
_ [EPha](http://download.epha.ch/data/matrix/matrix.csv)\
_ [Swisspeddose](https://swisspeddose.ch)\
_ [Sappinfo](https://sappinfo.ch)
## Output Sqlite Database
_ [amiko_db_de](http://pillbox.oddb.org/amiko_db_full_idx_de.zip)\
_ [amiko_db_fr](http://pillbox.oddb.org/amiko_db_full_idx_fr.zip)
## Glossary
_ [GTIN](http://www.ywesee.com/Main/EANCode)
