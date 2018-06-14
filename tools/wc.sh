#!/bin/sh

HERE=$( cd $(dirname $0) && pwd )
cd ${HERE}/..

wc $(find src/ \( -name "*h" -o -name "*cpp" -o -name "*c" \) -a ! -path 'src/rpc/*' )
