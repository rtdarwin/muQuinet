#!/bin/bash

HERE=$( cd $(dirname $0) && pwd )

BUILD_DIR=${HERE}/../build.debug
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

cmake -DCMAKE_BUILD_TYPE=Debug \
      -DUSE_ADDRESS_SANITIZER=OFF -DUSE_THREAD_SANITIZER=OFF ..
make
