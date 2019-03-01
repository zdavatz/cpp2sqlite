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

rm -rf $BLD_DIR
mkdir -p $BLD_DIR
cd $BLD_DIR

cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$BIN_DIR \
    -DXLNT_DIR=/usr/local \
    -D XLNT_LIBRARY_DIRS=/usr/local \
    $SRC_DIR

make -j9
sudo make install

cd $BLD_DIR  # it should be $BIN_DIR
time cpp2sqlite --verbose --lang=fr --inDir $SRC_DIR/input
#time cpp2sqlite --verbose --inDir $HOME/.software/cpp2sqlite/input
