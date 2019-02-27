#!/bin/bash
# Alex Bettarini - 22 Jan 2019

WD=$(pwd)
SRC_DIR=../
if [ ! -z "$1" ] ; then
    SRC_DIR=$1
fi

BLD_DIR=$SRC_DIR/build
BIN_DIR=$BLD_DIR

# Issue #3 specifications:
DOWNLOAD_DIR=$SRC_DIR/download
OUTPUT_DIR=$SRC_DIR/output

mkdir -p $OUTPUT_DIR
mkdir -p $DOWNLOAD_DIR ; cd $DOWNLOAD_DIR

#echo "SRC_DIR: $SRC_DIR"
#echo "DOWNLOAD_DIR: $DOWNLOAD_DIR"
#echo "OUTPUT_DIR: $OUTPUT_DIR"

#-------------------------------------------------------------------------------
# swissmedic
FILE1="https://www.swissmedic.ch/dam/swissmedic/de/dokumente/internetlisten/zugelassene_packungen_ham.xlsx.download.xlsx/Zugelassene_Packungen%20HAM_31012019.xlsx"
wget -N $FILE1 -O swissmedic_packages.xlsx

#-------------------------------------------------------------------------------
# refdata
# see Java code, file: AllDown.java:254 function: downRefdataPharmaXml
# https://dev.ywesee.com/Refdata/Wget
# http://refdatabase.refdata.ch/Service/Article.asmx?op=Download

URL="http://refdatabase.refdata.ch"  # article and partner

wget --post-file "$WD/ref.xml" \
    --header "content-type: text/xml;charset=utf-8" \
    --header "SOAPAction: $URL/Pharma/Download" \
    "$URL/Service/Article.asmx" -O temp.xml

xmllint --format temp.xml > refdata_pharma.xml
rm temp.xml

#-------------------------------------------------------------------------------
# bag
wget http://bag.e-mediat.net/SL2007.Web.External/File.axd?file=XMLPublications.zip -O XMLPublications.zip
unzip XMLPublications.zip Preparations.xml
mv -f Preparations.xml bag_preparations.xml
rm XMLPublications.zip
exit 0

#-------------------------------------------------------------------------------
# swisspeddose
# https://db.swisspeddose.ch/app/uploads/xml_publication/swisspeddosepublication-2019-02-21.xml
# https://db.swisspeddose.ch

FILE3=""
wget -N $FILE3

#-------------------------------------------------------------------------------
# AIPS
URL="http://download.swissmedicinfo.ch"

POST_DATA="ctl00$MainContent$btnOK=Ja, ich akzeptiere / Oui, j’accepte / Sì, accetto"
#POST_DATA="ctl00$MainContent$BtnYes=Ja"

wget --post-data=$POST_DATA \
	"$URL/Accept.aspx" --save-cookies cookie.txt --keep-session-cookies --delete-after

#wget --load-cookies=cookie.txt '$URL?AgreementSessionObject=Agree'
#wget --load-cookies=cookie.txt http://download.swissmedicinfo.ch/default.aspx
#wget --load-cookies=cookie.txt http://download.swissmedicinfo.ch
