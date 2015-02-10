#!/bin/bash
#cd bld
#make -j5
#cd ..
mkdir -p tests/outputs
./test_use_base.sh
./regression_tester.rb ./dcc_original -s -c 2>stderr >stdout; diff -wB tests/prev/ tests/outputs/
