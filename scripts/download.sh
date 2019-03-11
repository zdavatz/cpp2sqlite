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

#-------------------------------------------------------------------------------
# epha no longer needed
#wget -N http://download.epha.ch/cleaned/produkte.json -O epha_products_de_json.json

#-------------------------------------------------------------------------------
# swissmedic
FILE1="https://www.swissmedic.ch/dam/swissmedic/de/dokumente/internetlisten/zugelassene_packungen_ham.xlsx"
wget -N $FILE1 -O swissmedic_packages.xlsx

#-------------------------------------------------------------------------------
# refdata
# see Java code, file: AllDown.java:254 function: downRefdataPharmaXml
# https://dev.ywesee.com/Refdata/Wget
# http://refdatabase.refdata.ch/Service/Article.asmx?op=Download

URL="http://refdatabase.refdata.ch"  # article and partner
TARGET=refdata_pharma.xml

wget --post-file "$WD/ref.xml" \
    --header "content-type: text/xml;charset=utf-8" \
    --header "SOAPAction: $URL/Pharma/Download" \
    "$URL/Service/Article.asmx" -O temp.xml

xmllint --format temp.xml > $TARGET

# Clean up soap tags, and xmlns
sed -i -e '/<soap:/d' $TARGET
sed -i -e '/<\/soap:/d' $TARGET
sed -i -e 's/xmlns="[^"]*"//' $TARGET

rm temp.xml
rm $TARGET-e

#-------------------------------------------------------------------------------
# bag
wget http://bag.e-mediat.net/SL2007.Web.External/File.axd?file=XMLPublications.zip -O XMLPublications.zip
unzip XMLPublications.zip Preparations.xml
mv -f Preparations.xml bag_preparations.xml
rm XMLPublications.zip

#-------------------------------------------------------------------------------
# swisspeddose

if [ ! -f $WD/passwords ] ; then
    echo "swisspeddose passwords file not found"
else
source $WD/passwords
URL="https://db.swisspeddose.ch"
FILE3="$URL/app/uploads/xml_publication/swisspeddosepublication-2019-02-21.xml"

#POST_DATA='Submit=Log In&log=${USERNAME}&pwd=${PASSWORD}'
POST_DATA='log=${USERNAME}&pwd=${PASSWORD}'
#  <input type="submit" name="Submit" value="Log In" class="buttons" />

wget --save-cookies cookies.txt \
     --keep-session-cookies \
     --delete-after \
     --post-data=$POST_DATA \
     "$URL/sign-in"

# | escaped to %7C
COOKIE1='_pk_id.1.308c=c078669ae31d3522.1551272544.2.1551288031.1551287960.'
COOKIE2='spd_terms_accepted_=1'
COOKIE3='_pk_ses.1.308c=1'
COOKIE4="wordpress_logged_in_1880a7b331025031dc6ad3fbaba977cf=${USERNAME}%7C${PASSWORD}%7CNqC0nhspo8dhQgNCtUB52CDopnUhn5FXs0R7wukyFI1%7C5ceeab44337c84cf3f25997cb96b854fba5fbf14562b5c04edf92a730088e1b7"
COOKIE5='_icl_current_language=en'
wget --header "Host: db.swisspeddose.ch" \
	--user-agent "Mozilla/5.0 (X11; Linux x86_64; rv:65.0) Gecko/20100101 Firefox/65.0" \
	--header "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8" \
	--header "Accept-Language: de,en-US;q=0.7,en;q=0.3" \
	--referer "$URL/dashboard/" \
	--header "Cookie: ${COOKIE1}; ${COOKIE2}; ${COOKIE3}; ${COOKIE4}; ${COOKIE5}" \
	--header "Upgrade-Insecure-Requests: 1" \
	$FILE3 -O swisspeddosepublication.xml

rm cookies.txt
fi

#-------------------------------------------------------------------------------
# AIPS

URL="http://download.swissmedicinfo.ch"
TARGET=AipsDownload.zip

# First get index.html and the first cookie
wget --save-cookies cookieA.txt --keep-session-cookies $URL

VS_VALUE=$(grep -w __VIEWSTATE index.html | awk '{print $5}' | sed 's/value=//g' | sed 's/"//g')
EVVAL_VALUE=$(grep -w __EVENTVALIDATION index.html | awk '{print $5}' | sed 's/value=//g' | sed 's/"//g')

# urlencode VS_VALUE and VS_VALUE, alternatively use curl with --data-urlencode
VS_VALUE=$(urlencode $VS_VALUE)
EVVAL_VALUE=$(urlencode $EVVAL_VALUE)

VS="__VIEWSTATE=$VS_VALUE"
EVVAL="__EVENTVALIDATION=$EVVAL_VALUE"
VSG="__VIEWSTATEGENERATOR=00755B7A"
BTN_OK="ctl00%24MainContent%24btnOK=Ja%2C+ich+akzeptiere+%2F+Oui%2C+j%E2%80%99accepte+%2F+S%C3%AC%2C+accetto"
BODY_DATA="${VS}&${VSG}&${EVVAL}&${BTN_OK}"

# Then get index2.html and the second cookie
wget --header 'Host: download.swissmedicinfo.ch' \
    --user-agent 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_3) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/12.0.3' \
	--header 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8' \
   	--referer 'http://download.swissmedicinfo.ch/Accept.aspx?ReturnUrl=%2f' \
    --load-cookies=cookieA.txt \
    --save-cookies cookieB.txt --keep-session-cookies \
    --header 'Upgrade-Insecure-Requests: 1' \
    --post-data="$BODY_DATA" \
	"$URL/Accept.aspx?ReturnUrl=%2f" -O index2.html

# Extract data from index2.html hidden fields to be used in POST
VS_VALUE=$(grep -w __VIEWSTATE index2.html | awk '{print $5}' | sed 's/value=//g' | sed 's/"//g')
EVVAL_VALUE=$(grep -w __EVENTVALIDATION index2.html | awk '{print $5}' | sed 's/value=//g' | sed 's/"//g')

# urlencode VS_VALUE and VS_VALUE, alternatively use curl with --data-urlencode
VS_VALUE=$(urlencode $VS_VALUE)
EVVAL_VALUE=$(urlencode $EVVAL_VALUE)

VS="__VIEWSTATE=$VS_VALUE"
EVVAL="__EVENTVALIDATION=$EVVAL_VALUE"
VSG="__VIEWSTATEGENERATOR=CA0B0334"
BTN_YES="ctl00%24MainContent%24BtnYes=Ja"
BODY_DATA="${VS}&${VSG}&${EVVAL}&${BTN_YES}"

wget --header 'Host: download.swissmedicinfo.ch' \
	--user-agent 'Mozilla/5.0 (X11; Linux x86_64; rv:65.0) Gecko/20100101 Firefox/65.0' \
	--header 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8' \
	--header 'Accept-Language: de,en-US;q=0.7,en;q=0.3' \
	--referer 'http://download.swissmedicinfo.ch/' \
	--header 'Content-Type: application/x-www-form-urlencoded' \
    --load-cookies=cookieA.txt \
    --load-cookies=cookieB.txt \
	--header 'Upgrade-Insecure-Requests: 1' \
	--method POST \
    --body-data "$BODY_DATA" \
	$URL \
	--output-document $TARGET

file --brief $TARGET | grep "Zip archive data" # Check that we got a zip file with:
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

