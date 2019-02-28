#!/bin/bash
# Alex Bettarini - 22 Jan 2019

WD=$(pwd)
SRC_DIR=../

if [ "$#" -lt 2 ] ; then
    echo -e "Usage:\n\t$0 username password"
fi

if [ ! -z "$3" ] ; then
    SRC_DIR=$3
fi

USERNAME=$1
PASSWORD=$2

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
# epha no longer needed
#wget -N http://download.epha.ch/cleaned/produkte.json -O epha_products_de_json.json

#-------------------------------------------------------------------------------
# swissmedic
# TODO: the timestamp could change
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

#-------------------------------------------------------------------------------
# swisspeddose

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

#-------------------------------------------------------------------------------
# AIPS

URL="http://download.swissmedicinfo.ch"

# <input type="submit" name="ctl00$MainContent$btnOK" value="Ja, ich akzeptiere / Oui, j’accepte / Sì, accetto" id="MainContent_btnOK" />
POST_DATA="ctl00$MainContent$btnOK=Ja, ich akzeptiere / Oui, j’accepte / Sì, accetto"
#POST_DATA="ctl00%24MainContent%24btnOK=Ja, ich akzeptiere / Oui, j’accepte / Sì, accetto"
#POST_DATA="ctl00$MainContent$btnOK"
wget --post-data=$POST_DATA \
    --user-agent 'Mozilla/5.0 (X11; Linux x86_64; rv:65.0) Gecko/20100101 Firefox/65.0' \
	"$URL" --save-cookies cookie.txt --keep-session-cookies

# TODO: Extract data from index.html hidden fields to be used in POST
# / becomes %2F
# = becomes %3D
# $ becomes %24
VS="__VIEWSTATE=%2FwEPDwULLTE4MTg5MDc1ODUPZBYCZg9kFgICAw9kFgICCw8WAh4LXyFJdGVtQ291bnQCBBYIZg9kFgRmDxUBCD9MYW5nPURFZAIBDw8WCB4EVGV4dAUCREUeB1Rvb2xUaXAFAkRFHglGb250X0JvbGRnHgRfIVNCAoAQZGQCAg9kFgRmDxUBCD9MYW5nPUZSZAIBDw8WCB8BBQJGUh8CBQJGUh8DaB8EAoAQZGQCBA9kFgRmDxUBCD9MYW5nPUlUZAIBDw8WCB8BBQJJVB8CBQJJVB8DaB8EAoAQZGQCBg9kFgRmDxUBCD9MYW5nPUVOZAIBDw8WCB8BBQJFTh8CBQJFTh8DaB8EAoAQZGRkjlURDuFTfUSzCkU4Z%2FHgzJTwh8JAx9UG3UUmb1vpCvk%3D"
VSG="__VIEWSTATEGENERATOR=CA0B0334"
EVVAL="__EVENTVALIDATION=%2FwEdAAP75S3dOZ6sf0G6ruN2nzl0Ys46t3fbHVLzSTNxrdjIh8RrVmh3M0KCdbB64e6QzAIEClHNrqZ9LNHZdrtCdxbx9WpxB6%2BPdFanYBbOaZT3Pw%3D%3D"
BTN_YES="ctl00%24MainContent%24BtnYes=Ja"
BODY_DATA="${VS}&${VSG}&${EVVAL}&${BTN_YES}"

COOKIE1='ASP.NET_SessionId=tq1lpqudjurbf41p53vkxq3d' # in cookie.txt
COOKIE2='.ASPXAUTH=F8371FB0AD90759CC025EC3D8236112BD865A3210322FABE16AD5895423FA5EE1E6FBDDCDA9C1C2CD2C9B0B5F9BD963BBEB00A0B19B59A195A4116FDC749D59953DC76138E28834E4CFB498F60C7C28359DB889E1A1D2052AA5AA98614169939327FAA374D22FE8083FD3C35FBB4D9B418639D2FB835F36C41C2D238BADAC35B'
#COOKIES="Cookie: ${COOKIE1}; ${COOKIE2}"
COOKIES="Cookie: ${COOKIE2}"

wget --header 'Host: download.swissmedicinfo.ch' \
	--user-agent 'Mozilla/5.0 (X11; Linux x86_64; rv:65.0) Gecko/20100101 Firefox/65.0' \
	--header 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8' \
	--header 'Accept-Language: de,en-US;q=0.7,en;q=0.3' \
	--referer 'http://download.swissmedicinfo.ch/' \
	--header 'Content-Type: application/x-www-form-urlencoded' \
    --load-cookies=cookie.txt \
	--header "$COOKIES" \
	--header 'Upgrade-Insecure-Requests: 1' \
	--method POST \
    --body-data "$BODY_DATA" \
	$URL \
	--output-document 'AipsDownload.zip'

# TODO: unzip AipsDownload.zip

rm index.html
rm cookie.txt

