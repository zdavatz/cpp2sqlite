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
- [xlnt](https://github.com/tfussell/xlnt) with `cmake -DSTATIC=on`, also apply [swissmedic.patch](https://github.com/zdavatz/cpp2sqlite/files/3584890/swissmedic.patch.txt)
- jq (Command-line JSON processor)
- [json](https://github.com/nlohmann/json.git)

## Installation
$ git clone\
$ git submodule init\
$ git submodule update

See [build.sh](https://github.com/zdavatz/cpp2sqlite/blob/master/scripts/build.sh)
## Usage
### cpp2sqlite
`./cpp2sqlite --inDir ~/.software/cpp2sqlite/input`
### deepl translation
_ run `build/sappinfo` to generate the unique language file.\
_ comment in the line 20 of [build.sh](https://github.com/zdavatz/cpp2sqlite/blob/master/scripts/build.sh#L20) to do the translations.\
_ if there are no errors, then commit `intput/deepl.sappinfo.out.fr.txt`.
## Input Sources
_ [AIPS](http://download.swissmedicinfo.ch)\
_ [BAG XML](http://www.spezialitätenliste.ch/File.axd?file=XMLPublications.zip)\
_ [Refdata Artikel](https://www.refdata.ch/de/artikel/abfrage/artikel-refdatabase-gtin)\
_ [Refdata Partner](https://www.refdata.ch/de/partner/abfrage/partner-refdatabase-gln)\
_ [Refdata SAI](https://sai.refdata.ch/download)\
_ [Swissmedic](https://www.swissmedic.ch/dam/swissmedic/de/dokumente/listen/excel-version_zugelasseneverpackungen.xlsx.download.xlsx/excel-version_zugelasseneverpackungen.xlsx)\
_ [Swissmedic HPC](https://www.swissmedic.ch/swissmedic/de/home/humanarzneimittel/marktueberwachung/health-professional-communication--hpc-.html)\
_ [Swissmedic Chargenrückrufe](https://www.swissmedic.ch/swissmedic/de/home/humanarzneimittel/marktueberwachung/qualitaetsmaengel-und-chargenrueckrufe/chargenrueckrufe.html)\
_ [EPha](http://download.epha.ch/data/matrix/matrix.csv)\
_ [Swisspeddose](https://swisspeddose.ch)\
_ [Sappinfo](https://sappinfo.ch)
_ [Drugshortage](https://Drugshortage)
## Output Sqlite Database
_ [amiko_db_de](http://pillbox.oddb.org/amiko_db_full_idx_de.zip)\
_ [amiko_db_fr](http://pillbox.oddb.org/amiko_db_full_idx_fr.zip)
## Glossary
_ [GTIN](http://www.ywesee.com/Main/EANCode)
