#!/bin/bash
cd bld
make -j5
cd ..
./test_use_base.sh
./valgrind_tester ./bld/dcc_original -s -c 2>stderr >stdout; diff tests/prev/ tests/outputs/
