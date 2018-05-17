#!/bin/bash

HERE=$( cd $(dirname $0) && pwd )
INTERCEPTOR_DSO="${HERE}/../lib/libmuquinet_interceptor.so"
prog="${1}"

export LD_PRELOAD="/usr/lib/x86_64-linux-gnu/libasan.so.4 ${INTERCEPTOR_DSO}"
shift
${prog} $@
