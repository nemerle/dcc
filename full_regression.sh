#!/bin/bash
./test_use_all.sh
./regression_tester.rb ./bld/dcc_original -s -c 2>stderr >stdout; diff tests/prev/ tests/outputs/
