#!/bin/bash

#STEP_GIT=true
STEP_DOWNLOAD=true

#STEP_CONFIGURE_XLNT=true
#STEP_BUILD_XLNT=true

#STEP_CONFIGURE_JSON=true
#STEP_BUILD_JSON=true

STEP_BUILD_APPS=true

STEP_RUN_C2S=true
STEP_RUN_PHARMA=true
#STEP_DEEPL_INTERACTIONS=true
#STEP_DEEPL_SAPPINFO=true

#-------------------------------------------------------------------------------
SRC_DIR=$(pwd)/..
BLD_DIR="$SRC_DIR/build"
BIN_DIR=/usr/local/bin

SRC_XLNT=$SRC_DIR/xlnt
SRC_JSON=$SRC_DIR/json

BLD_XLNT=$BLD_DIR/xlnt
BLD_JSON=$BLD_DIR/json
BLD_APPS=$BLD_DIR

BIN_XLNT=$BIN_DIR
BIN_JSON=$BIN_DIR
BIN_APPS=$BIN_DIR

#-------------------------------------------------------------------------------
if [ $STEP_GIT ] ; then
pushd ../
git pull        # dangerous: it could update this build.sh file
git submodule init
git submodule update
popd
fi

#-------------------------------------------------------------------------------
if [ $STEP_DOWNLOAD ] ; then
./download.sh
fi

#-------------------------------------------------------------------------------
if [ $STEP_CONFIGURE_XLNT ] ; then
mkdir -p $BLD_XLNT ; cd $BLD_XLNT
$CMAKE -G"$GENERATOR" \
	-D STATIC=ON \
	-D SAMPLES=ON \
	-D CMAKE_INSTALL_PREFIX=$BIN_XLNT \
	-D CMAKE_BUILD_TYPE=$BUILD_TYPE \
	$SRC_XLNT
fi

if [ $STEP_BUILD_XLNT ] ; then
	cd $BLD_XLNT
	make $MAKE_FLAGS
    make install
fi

#------------------------------------------------------------------------------
if [ $STEP_CONFIGURE_JSON ] ; then
mkdir -p $BLD_JSON ; cd $BLD_JSON
$CMAKE -G"$GENERATOR" \
	-D CMAKE_INSTALL_PREFIX=$BIN_JSON \
	-D CMAKE_BUILD_TYPE=$BUILD_TYPE \
	$SRC_JSON
fi

if [ $STEP_BUILD_JSON ] ; then
	cd $BLD_JSON
	make $MAKE_FLAGS
    make install
fi

#-------------------------------------------------------------------------------
if [ $STEP_BUILD_APPS ] ; then
rm -rf $BLD_APPS
mkdir -p $BLD_APPS ; cd $BLD_APPS
cmake -G"Unix Makefiles" \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_INSTALL_PREFIX=$BIN_APPS \
    -D XLNT_DIR=$BIN_XLNT \
	-D JSON_DIR=$BIN_JSON \
    $SRC_DIR

make -j9 all
#sudo make install
fi

#-------------------------------------------------------------------------------
if [ $STEP_RUN_C2S ] ; then
cd $BLD_APPS  # it should be $BIN_APPS otherwise there is no point in doing make install
time ./cpp2sqlite --verbose --lang=fr --inDir $SRC_DIR/input&
time ./cpp2sqlite --verbose --inDir $SRC_DIR/input&
fi

#-------------------------------------------------------------------------------
if [ $STEP_RUN_PHARMA ] ; then
cd $BLD_APPS
./pharma --inDir $SRC_DIR/input
fi

#-------------------------------------------------------------------------------
if [ $STEP_DEEPL_INTERACTIONS ] ; then
# STEP 1
# Read file:
#       input/matrix.csv
# Write files:
#       output/drug_interactions_de.csv
#       input/deepl.in.txt
#./interaction --inDir $SRC_DIR/input

# STEP 2 for each language other than "de"
export TARGET_LANG=fr
# Read file:
#       input/deepl.in.txt
# Write files:
#       input/deepl.out.$TARGET_LANG.txt
#       input/deepl.err.$TARGET_LANG.txt
#./deepl.sh

# STEP 3
# Manual translation
# Read file:
#       input/deepl.err.$TARGET_LANG.txt
# Write file:
#       input/deepl.out2.$TARGET_LANG.txt

# STEP 4
# Read files:
#       input/matrix.csv
#       input/deepl.in.txt
#       input/deepl.out.$TARGET_LANG.txt
#       input/deepl.out2.$TARGET_LANG.txt
#       input/atc_codes_multi_lingual.txt
# Write file:
#       output/drug_interactions_$TARGET_LANG.csv
if [ "$(wc -l < $SRC_DIR/input/deepl.in.txt)" -ne "$(wc -l < $SRC_DIR/input/deepl.out.$TARGET_LANG.txt)" ]; then
    echo 'deepl.in.txt and deepl.out.$TARGET_LANG.txt must have the same number of lines'
else
    ./interaction --inDir $SRC_DIR/input --lang $TARGET_LANG
fi
fi

#-------------------------------------------------------------------------------
if [ $STEP_DEEPL_SAPPINFO ] ; then
JOB=sappinfo
# STEP 1
# Read file:
#       input/sappinfo.xlsx
# Write files:
#       input/deepl.${JOB}.in.txt
#./sappinfo --inDir $SRC_DIR/input

# STEP 2 for each language other than "de"
export TARGET_LANG=fr
# Read file:
#       input/deepl.${JOB}.in.txt
# Write files:
#       input/deepl.${JOB}.out.$TARGET_LANG.txt
#       input/deepl.${JOB}.err.$TARGET_LANG.txt
./deepl.sh $SRC_DIR/input $JOB

# STEP 3
# Manual translation
# Read file:
#       input/deepl.${JOB}.err.$TARGET_LANG.txt
# Write file:
#       input/deepl.${JOB}.out2.$TARGET_LANG.txt

# STEP 4
# Validate translated files
if [ "$(wc -l < $SRC_DIR/input/deepl.${JOB}.in.txt)" -ne "$(wc -l < $SRC_DIR/input/deepl.${JOB}.out.$TARGET_LANG.txt)" ]; then
    echo 'deepl.${JOB}.in.txt and deepl.${JOB}.out.$TARGET_LANG.txt must have the same number of lines'
fi
fi

