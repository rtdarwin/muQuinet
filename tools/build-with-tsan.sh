#!/bin/bash

HERE=$( cd $(dirname $0) && pwd )

BUILD_DIR=${HERE}/../build.debug
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# if [ ! -e ${BUILD_DIR}/Makefile ]; then
cmake -DCMAKE_BUILD_TYPE=Debug -DUSE_THREAD_SANITIZER=ON ..
# fi
make
