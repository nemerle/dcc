#!/bin/bash
makedir -p tests/outputs
./test_use_all.sh
./regression_tester.rb ./dcc_original -s -c 2>stderr >stdout; diff -wB tests/prev/ tests/outputs/
