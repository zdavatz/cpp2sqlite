#!/bin/bash
# Alex Bettarini - 22 Jan 2019

# Usage: before running this file do
#   either
#       source steps_public1.source
#   or
#       source steps_public2.source

#STEP_DOWNLOAD_EPHA=true

WD=$(pwd)
SRC_DIR=$(realpath ../)

if [ ! -z "$1" ] ; then
    SRC_DIR=$(realpath $1)
fi

BLD_DIR=$SRC_DIR/build
BIN_DIR=$BLD_DIR

# Issue #3 specifications:
DOWNLOAD_DIR=$SRC_DIR/downloads
OUTPUT_DIR=$SRC_DIR/output

mkdir -p $OUTPUT_DIR
mkdir -p $DOWNLOAD_DIR ; cd $DOWNLOAD_DIR

#echo "SRC_DIR: $SRC_DIR"
#echo "DOWNLOAD_DIR: $DOWNLOAD_DIR"
#echo "OUTPUT_DIR: $OUTPUT_DIR"

#-------------------------------------------------------------------------------
function urlencode() {
# urlencode <string>
    old_lc_collate=$LC_COLLATE
    LC_COLLATE=C
local length="${#1}"
for (( i = 0; i < length; i++ )); do
local c="${1:i:1}"
case $c in
            [a-zA-Z0-9.~_-]) printf "$c" ;;
*) printf '%%%02X' "'$c" ;;
esac
done
    LC_COLLATE=$old_lc_collate
}

# https://gist.github.com/hrwgc/7455343
function validate_url() {
  if [[ `wget -S --spider $1 2>&1 | grep 'HTTP/1.1 200 OK'` ]]; then
    return 0
  else
    return 1
  fi
}

#-------------------------------------------------------------------------------
# epha no longer needed

if [ $STEP_DOWNLOAD_EPHA ] ; then
wget -N http://download.epha.ch/cleaned/produkte.json -O epha_products_de_json.json
fi

#-------------------------------------------------------------------------------

if [ $STEP_DOWNLOAD_SWISSMEDIC_PACKAGES ] ; then
# Needed in AmiKo
URL="https://www.swissmedic.ch"
FILE1="$URL/dam/swissmedic/de/dokumente/internetlisten/zugelassene_packungen_human.xlsx.download.xlsx/zugelassene_packungen_ham.xlsx"

if validate_url $URL && validate_url $FILE1 ; then
    TEMP_FILE=swissmedic_packages_temp.xlsx

    # To be extra cautious remove previously downloaded stuff, so there is no way it ends up being reused
    if [ -f $TEMP_FILE ] ; then
        rm $TEMP_FILE
    fi
    
    # By now we know that $FILE1 is available. Try to download it with a different filename, before it can be validated
    wget -N $FILE1 -O $TEMP_FILE

    # Issue 126 check integrity of downloaded xlsx (ZIP) file
    unzip -t -qq $TEMP_FILE
    retVal=$?
    if [ $retVal -ne 0 ] ; then
        echo "Error $retVal downloading swissmedic_packages.xlsx"
        if [ ! -f swissmedic_packages.xlsx ] ; then
            echo "First download: abort !"
            exit $retVal
        fi
        echo "Use old file: swissmedic_packages.xlsx"
        rm $TEMP_FILE
    else
        # Download and validation successful. Give it the intended name, possibly overwriting previous download
        mv -f $TEMP_FILE swissmedic_packages.xlsx
    fi    
else
    echo "Swissmedic currently not available"
fi

fi

if [ $STEP_DOWNLOAD_SWISSMEDIC_HAM ] ; then
# Needed in pharma
# Extended drug list
FILE2="https://www.swissmedic.ch/dam/swissmedic/de/dokumente/internetlisten/erweiterte_ham.xlsx.download.xlsx/Erweiterte_Arzneimittelliste%20HAM.xlsx"
wget -N $FILE2
fi

#-------------------------------------------------------------------------------
# see Java code, file: AllDown.java:254 function: downRefdataPharmaXml
# https://dev.ywesee.com/Refdata/Wget
# http://refdatabase.refdata.ch/Service/Article.asmx?op=Download

if [ $STEP_DOWNLOAD_REFDATA ] ; then

URL="https://refdatabase.refdata.ch"  # article and partner
TARGET=refdata_pharma.xml

wget --post-file "$WD/ref.xml" \
    --header "content-type: text/xml;charset=utf-8" \
    --header "SOAPAction: http://refdatabase.refdata.ch/Pharma/Download" \
    "$URL/Service/Article.asmx" -O temp.xml

xmllint --format temp.xml > $TARGET

# Clean up soap tags, and xmlns
sed -i -e '/<soap:/d' $TARGET
sed -i -e '/<\/soap:/d' $TARGET
sed -i -e 's/xmlns="[^"]*"//' $TARGET

rm temp.xml
rm $TARGET-e
fi

#-------------------------------------------------------------------------------

if [ $STEP_DOWNLOAD_BAG ] ; then
wget https://www.xn--spezialittenliste-yqb.ch/File.axd?file=XMLPublications.zip -O XMLPublications.zip
unzip XMLPublications.zip Preparations.xml
mv -f Preparations.xml bag_preparations.xml
rm XMLPublications.zip
fi

#-------------------------------------------------------------------------------

if [ $STEP_DOWNLOAD_SWISSPEDDOSE ] ; then

if [ ! -f $WD/passwords ] ; then
    echo "swisspeddose passwords file not found"
else
source $WD/passwords
URL="https://db.swisspeddose.ch"
POST_DATA="log=${USERNAME}&pwd=${PASSWORD}&a=login&Submit=Log+In"

echo "Logging in"

curl 'https://db.swisspeddose.ch/sign-in/' -X POST \
    --cookie-jar cookiesB.txt \
    -o signed-in.html \
    --data-raw $POST_DATA

echo "Getting dashboard"

curl -H 'User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:65.0) Gecko/20100101 Firefox/65.0' \
    -H "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" \
    -H "Accept-Language: de,en-US;q=0.7,en;q=0.3" \
    -H "Upgrade-Insecure-Requests: 1" \
    --referer "$URL/sign-in/" \
    --cookie cookiesB.txt \
    "$URL/dashboard/" -o dashboard.html

echo "Got dashboard"

FILENAME_V4=$(grep -o -E 'a href="/app/uploads/xml_publication/swisspeddosepublicationv4-[0-9]{4}-[0-9]{1,2}-[0-9]{1,2}.zip' dashboard.html | awk -F\" '{print $2}')

echo "swisspeddosepublication_v4 filename: $FILENAME_V4"

echo "Getting zip v4"
curl -H "Host: db.swisspeddose.ch" \
    -H "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:65.0) Gecko/20100101 Firefox/65.0" \
    -H "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" \
    -H "Accept-Language: de,en-US;q=0.7,en;q=0.3" \
    -H 'Upgrade-Insecure-Requests: 1' \
    --referer "Referer: $URL/dashboard/" \
    --cookie cookiesB.txt \
    -o 'swisspeddosepublication_v4.zip' \
    "${URL}${FILENAME_V4}"
echo "Got zip"

unzip "swisspeddosepublication_v4.zip"

rm "swisspeddosepublication_v4.zip"
mv SwissPedDosePublicationV4.xml "swisspeddosepublication_v4.xml"

rm cookies*.txt
rm dashboard.html
rm signed-in.html
fi
fi

#-------------------------------------------------------------------------------

if [ $STEP_DOWNLOAD_AIPS ] ; then
URL="https://download.swissmedicinfo.ch"
TARGET=AipsDownload.zip

# First get index.html and the first cookie
wget --save-cookies cookieA.txt --keep-session-cookies $URL

VS_VALUE=$(grep -w __VIEWSTATE index.html | awk '{print $5}' | sed 's/value=//g' | sed 's/"//g')
EVVAL_VALUE=$(grep -w __EVENTVALIDATION index.html | awk '{print $5}' | sed 's/value=//g' | sed 's/"//g')

# urlencode VS_VALUE and EVVAL_VALUE, alternatively use curl with --data-urlencode
VS_VALUE=$(urlencode $VS_VALUE)
EVVAL_VALUE=$(urlencode $EVVAL_VALUE)

## First Cookie can be skipped and this code can be deleted if first cookie does not come back.
#VS="__VIEWSTATE=$VS_VALUE"
#EVVAL="__EVENTVALIDATION=$EVVAL_VALUE"
#VSG="__VIEWSTATEGENERATOR=00755B7A"
#BTN_OK="ctl00%24MainContent%24btnOK=Ja%2C+ich+akzeptiere+%2F+Oui%2C+j%E2%80%99accepte+%2F+S%C3%AC%2C+accetto"
#BODY_DATA="${VS}&${VSG}&${EVVAL}&${BTN_OK}"
#
## Then get index2.html and the second cookie
#wget --header 'Host: download.swissmedicinfo.ch' \
#    --user-agent 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_3) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/12.0.3' \
#	--header 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8' \
#   	--referer '$URL/Accept.aspx?ReturnUrl=%2f' \
#    --load-cookies=cookieA.txt \
#    --save-cookies cookieB.txt --keep-session-cookies \
#    --header 'Upgrade-Insecure-Requests: 1' \
#    --post-data="$BODY_DATA" \
#	"$URL/Accept.aspx?ReturnUrl=%2f" -O index2.html
#
## Extract data from index2.html hidden fields to be used in POST
#VS_VALUE=$(grep -w __VIEWSTATE index2.html | awk '{print $5}' | sed 's/value=//g' | sed 's/"//g')
#EVVAL_VALUE=$(grep -w __EVENTVALIDATION index2.html | awk '{print $5}' | sed 's/value=//g' | sed 's/"//g')
#
## urlencode VS_VALUE and EVVAL_VALUE, alternatively use curl with --data-urlencode
#VS_VALUE=$(urlencode $VS_VALUE)
#EVVAL_VALUE=$(urlencode $EVVAL_VALUE)

VS="__VIEWSTATE=$VS_VALUE"
EVVAL="__EVENTVALIDATION=$EVVAL_VALUE"
VSG="__VIEWSTATEGENERATOR=CA0B0334"
BTN_YES="ctl00%24MainContent%24BtnYes=Ja"
BODY_DATA="${VS}&${VSG}&${EVVAL}&${BTN_YES}"

wget --header 'Host: download.swissmedicinfo.ch' \
	--user-agent 'Mozilla/5.0 (X11; Linux x86_64; rv:65.0) Gecko/20100101 Firefox/65.0' \
	--header 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8' \
	--header 'Accept-Language: de,en-US;q=0.7,en;q=0.3' \
	--referer '$URL/' \
	--header 'Content-Type: application/x-www-form-urlencoded' \
    --load-cookies=cookieA.txt \
    --load-cookies=cookieB.txt \
	--header 'Upgrade-Insecure-Requests: 1' \
	--method POST \
    --body-data "$BODY_DATA" \
	$URL \
	--output-document $TARGET

file $TARGET | grep "Zip archive data" # Check that we got a zip file with:
RESULT=$?
if [ $RESULT -ne 0 ] ; then
    file --brief $TARGET
    echo -e "$TARGET is not a zip file"
else
    unzip $TARGET -d temp
    mv "$(ls temp/AipsDownload_*.xml)" aips.xml
    rm -r temp
    rm $TARGET
    rm index*
    rm cookie*.txt
fi
fi

if [ $STEP_DOWNLOAD_SAI ] ; then
FILE_PATH=$(curl https://sai.refdata.ch/download | grep -Eo '/download/structuredexportzip/[^"]+' | head -n 1)
URL="https://sai.refdata.ch$FILE_PATH"
TARGET="sai.zip"

wget "$URL" --output-document $TARGET

file $TARGET | grep "Zip archive data" # Check that we got a zip file with:
RESULT=$?
if [ $RESULT -ne 0 ] ; then
    file --brief $TARGET
    echo -e "$TARGET is not a zip file"
else
    ls -lah
    rm -r SAI
    unzip -o $TARGET -d temp
    mv -f temp/SAI SAI
    rm -r temp
fi
fi

if [ $STEP_DOWNLOAD_NONPHARMA ] ; then
    wget https://www.firstbase.ch/sites/default/files/2023-11/firstbase_healthcare_public_data-dump%2C%2020231109.xlsx -O nonpharma.xlsx
fi

if [ $STEP_DOWNLOAD_TRANSFER_ZIP ] ; then
  curl -o "transfer.zip" "http://pillbox.oddb.org/TRANSFER.ZIP"
  unzip "transfer.zip" -d transfer
  rm transfer.zip
fi

