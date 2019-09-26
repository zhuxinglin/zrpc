#!/bin/bash

CUR_DIR=`pwd`
BUILD_DIR=${CUR_DIR}/build
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake ../
make
