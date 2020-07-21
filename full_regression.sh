#!/bin/bash
makedir -p tests/outputs
./test_use_all.sh
./regression_tester ./dcc_original -s -c; diff -wB tests/prev/ tests/outputs/
