#!/bin/bash

pushd ../
git pull        # dangerous: it could update this build.sh file
git submodule init
git submodule update
popd

./download.sh

SRC_DIR=$(pwd)/..
BLD_DIR="$SRC_DIR/build"
BIN_DIR=/usr/local/bin/

BIN_XLNT=/usr/local

rm -rf $BLD_DIR
mkdir -p $BLD_DIR
cd $BLD_DIR

cmake -G"Unix Makefiles" \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_INSTALL_PREFIX=$BIN_DIR \
    -D XLNT_DIR=$BIN_XLNT \
    $SRC_DIR

make -j9 all
#sudo make install

cd $BLD_DIR  # it should be $BIN_DIR otherwise there is no point in doing make install
time ./cpp2sqlite --lang=fr --inDir $SRC_DIR/input&
time ./cpp2sqlite --inDir $SRC_DIR/input&

#-------------------------------------------------------------------------------
# STEP 1
# Read file: input/matrix.csv
# Write files: output/drug_interactions_de.csv output/deepl.in.txt
#./interaction --inDir $SRC_DIR/input

LANG=fr
# STEP 2 for each language other than "de"
# Read file: output/deepl.in.txt
# Write files: output/deepl.out.$LANG.txt output/deepl.err.$LANG.txt
#./deepl.sh

# STEP 3
# Manually translate output/deepl.err.$LANG.txt to output/deepl.out2.$LANG.txt

# STEP 4
# Read files: input/matrix.csv output/deepl.in.txt output/deepl.out.$LANG.txt output/deepl.out2.$LANG.txt
# Write file: output/drug_interactions_$LANG.csv
./interaction --inDir $SRC_DIR/input --lang $LANG
