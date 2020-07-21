#!/bin/bash
#cd bld
#make -j5
#cd ..
mkdir -p tests/outputs
./test_use_base.sh
./regression_tester ./dcc_original -s -c; diff -wB tests/prev/ tests/outputs/
