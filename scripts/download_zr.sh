#!/bin/bash
# Alex Bettarini - 22 Jan 2019

WD=$(pwd)
SRC_DIR=$(realpath ../)

DOMAIN="ftp.zur-rose.ch"
if [ "test" == "$1" ] ; then
    DIR="/ywesee OutTest"
else
    DIR="/ywesee Out"
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
sshpass -p "$PASSWORD_ZUR" scp -v "$USERNAME_ZUR@$DOMAIN:$DIR/Artikelstamm.csv" artikel_stamm_zurrose.csv

# Updated every 4 hours
sshpass -p "$PASSWORD_ZUR" scp -v "$USERNAME_ZUR@$DOMAIN:$DIR/Artikelstamm_Voigt.csv" artikel_stamm_voigt.csv

# Updated every 30 minutes
ISO_8859_1_FILE=artikel_temp.csv
sshpass -p "$PASSWORD_ZUR" scp -v "$USERNAME_ZUR@$DOMAIN:$DIR/Artikelstamm_Vollstamm.csv" "$ISO_8859_1_FILE"
iconv -f ISO-8859-1 -t UTF-8 $ISO_8859_1_FILE >artikel_vollstamm_zurrose.csv
rm $ISO_8859_1_FILE

sshpass -p "$PASSWORD_ZUR" scp -v "$USERNAME_ZUR@$DOMAIN:$DIR/direktsubstitution.csv" "direct_subst_zurrose.csv"
sshpass -p "$PASSWORD_ZUR" scp -v "$USERNAME_ZUR@$DOMAIN:$DIR/Nota.csv" "nota_zurrose.csv"
sshpass -p "$PASSWORD_ZUR" scp -v "$USERNAME_ZUR@$DOMAIN:$DIR/Vollstamm_Galenic_Form_Mapping_by_Code.txt" "galenic_codes_map_zurrose.txt"

#curl -O "${URL}"/_log.txt --user "${USERNAME_ZUR}:${PASSWORD_ZUR}"
sshpass -p "$PASSWORD_ZUR" scp -v "$USERNAME_ZUR@$DOMAIN:$DIR/Autogenerika.csv" "Autogenerika.csv"

ISO_8859_1_FILE=Kunden_NEU_temp.csv
sshpass -p "$PASSWORD_ZUR" scp -v "$USERNAME_ZUR@$DOMAIN:$DIR/kunden_alle_dynamics_ce.csv" "$ISO_8859_1_FILE"

iconv -f ISO-8859-1 -t UTF-8 $ISO_8859_1_FILE >kunden_alle_dynamics_ce.csv
rm $ISO_8859_1_FILE

sshpass -p "$PASSWORD_ZUR" scp -v "$USERNAME_ZUR@$DOMAIN:$DIR/medix_kunden.csv" "medix_kunden.csv"

sshpass -p "$PASSWORD_ZUR" scp -v "$USERNAME_ZUR@$DOMAIN:$DIR/Grippeimpfstoff.csv" "Grippeimpfstoff.csv"

sshpass -p "$PASSWORD_ZUR" scp -v "$USERNAME_ZUR@$DOMAIN:$DIR/19er_pharmaCodes.csv" "19er_pharmaCodes.csv"

popd
