#!/bin/bash
# Alex Bettarini - 11 Mar 2019

source passwords
TARGET_LAN=fr
URL="https://api.deepl.com/v1"

IN_DIR=../input

if [ ! -z "$1" ] ; then
    IN_DIR=$1
fi

mkdir -p $IN_DIR
IN_FILE=$IN_DIR/deepl.in.txt
OUT_FILE=$IN_DIR/deepl.out.${TARGET_LAN}.txt
ERR_FILE=$IN_DIR/deepl.err.${TARGET_LAN}.txt
if [ -f $OUT_FILE ] ; then
    rm $OUT_FILE
fi

if [ -f $ERR_FILE ] ; then
    rm $ERR_FILE
fi

#-------------------------------------------------------------------------------
#wget -d -v "$URL/usage?auth_key=${AUTH_KEY}" -O - > temp.txt
#cat temp.txt

#-------------------------------------------------------------------------------
# Specifying - as filename for -O writes the output to stdout.

function translate() {
    local DATA=$1
    local GET_DATA="text=${DATA}&target_lang=${TARGET_LAN}&auth_key=${AUTH_KEY}"
    local POST_DATA="${GET_DATA}"
    local LENGTH=$(echo -n $POST_DATA | wc -c)
    #echo "data length: $LENGTH"
    #echo "translate $DATA"

if false; then
    wget  -d -v "$URL/translate?${GET_DATA}" -O - | jq -j '.translations[0].text' >> $OUT_FILE
else
    wget \
    --server-response \
    --header "Content-Type: application/x-www-form-urlencoded" \
    --header "Content-Length: $LENGTH" \
    --method POST \
    --body-data "$POST_DATA" \
    "$URL/translate" \
    -O - | jq -j '.translations[0].text' >> $OUT_FILE
fi

    if [ ${PIPESTATUS[0]} -ne 0 ] ; then
        echo -e "$DATA" >> $ERR_FILE
    fi
    echo "" >> $OUT_FILE
}

#-------------------------------------------------------------------------------

while read p; do
  translate "$p"
done < "$IN_FILE"

echo "Created $OUT_FILE and $ERR_FILE"
