# cpp2sqlite
C++ tool to generate sqlite database containing Swiss Healthcare Public Domain Drug Information
## History
This tool is a port of [aips2sqlite](https://github.com/zdavatz/aips2sqlite)
## Requirements
Using Boost, C++17 and [xlnt](https://github.com/tfussell/xlnt) with `cmake -DSTATIC=on`
## Installation
```
mkdir cpp2sqlite-build
cd cpp2sqlite-build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local/bin/ -DXLNT_DIR=/usr/local -DXLNT_LIBRARY_DIRS=/usr/local ../
make -j9
sudo make install
```
## Usage
`./cpp2sqlite --inDir /home/zeno/.software/aips2sqlite/jars/downloads/`
## Input Sources
_ [AIPS](http://download.swissmedicinfo.ch)\
_ [BAG XML](http://www.spezialitätenliste.ch/File.axd?file=XMLPublications.zip)\
_ [Refdata](https://www.refdata.ch/content/page_1.aspx?Nid=6&Aid=628&ID=291)\
_ [Swissmedic](https://www.swissmedic.ch/dam/swissmedic/de/dokumente/listen/excel-version_zugelasseneverpackungen.xlsx.download.xlsx/excel-version_zugelasseneverpackungen.xlsx)\
_ [EPha](http://download.epha.ch/data/matrix/matrix.csv)
## Output Sqlite Database
_ [amiko_db_de](http://pillbox.oddb.org/amiko_db_full_idx_de.zip)\
_ [amiko_db_fr](http://pillbox.oddb.org/amiko_db_full_idx_fr.zip)
## Glossary
_ [GTIN](http://www.ywesee.com/Main/EANCode)
