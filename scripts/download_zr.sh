#!/bin/bash
# Alex Bettarini - 22 Jan 2019

WD=$(pwd)
SRC_DIR=$(realpath ../)

if [ "test" == "$1" ] ; then
    URL="ftp://ftp.zur-rose.ch/ywesee OutTest"
else
    URL="ftp://ftp.zur-rose.ch/ywesee Out"
fi


BLD_DIR=$SRC_DIR/build
BIN_DIR=$BLD_DIR

#echo "SRC_DIR: $SRC_DIR"

#-------------------------------------------------------------------------------
# zurrose

source $WD/passwords
ZURROSE_DIR="${SRC_DIR}/input/zurrose"
#echo "ZURROSE_DIR: $ZURROSE_DIR"
mkdir -p $ZURROSE_DIR
pushd $ZURROSE_DIR

# Updated once per day
curl -o artikel_stamm_zurrose.csv -O "${URL}"/Artikelstamm.csv --user "${USERNAME_ZUR}:${PASSWORD_ZUR}"

# Updated every 4 hours
curl -o artikel_stamm_voigt.csv -O "${URL}"/Artikelstamm_Voigt.csv --user "${USERNAME_ZUR}:${PASSWORD_ZUR}"

# Updated every 30 minutes
ISO_8859_1_FILE=artikel_temp.csv
curl -o $ISO_8859_1_FILE -O "${URL}"/Artikelstamm_Vollstamm.csv --user "${USERNAME_ZUR}:${PASSWORD_ZUR}"
iconv -f ISO-8859-1 -t UTF-8 $ISO_8859_1_FILE >artikel_vollstamm_zurrose.csv
rm $ISO_8859_1_FILE

curl -o direct_subst_zurrose.csv -O "${URL}"/direktsubstitution.csv --user "${USERNAME_ZUR}:${PASSWORD_ZUR}"
curl -o nota_zurrose.csv -O "${URL}"/Nota.csv --user "${USERNAME_ZUR}:${PASSWORD_ZUR}"
curl -o galenic_codes_map_zurrose.txt -O "${URL}"/Vollstamm_Galenic_Form_Mapping_by_Code.txt --user "${USERNAME_ZUR}:${PASSWORD_ZUR}"

#curl -O "${URL}"/_log.txt --user "${USERNAME_ZUR}:${PASSWORD_ZUR}"
curl -O "${URL}"/Autogenerika.csv --user "${USERNAME_ZUR}:${PASSWORD_ZUR}"

ISO_8859_1_FILE=Kunden_temp.csv
curl -o $ISO_8859_1_FILE -O "${URL}"/Kunden_alle.csv --user "${USERNAME_ZUR}:${PASSWORD_ZUR}"
iconv -f ISO-8859-1 -t UTF-8 $ISO_8859_1_FILE >Kunden_alle.csv
rm $ISO_8859_1_FILE

ISO_8859_1_FILE=Kunden_NEU_temp.csv
curl -o $ISO_8859_1_FILE -O "${URL}"/Kunden_alle_NEU.csv --user "${USERNAME_ZUR}:${PASSWORD_ZUR}"
iconv -f ISO-8859-1 -t UTF-8 $ISO_8859_1_FILE >Kunden_alle_NEU.csv
rm $ISO_8859_1_FILE

popd


