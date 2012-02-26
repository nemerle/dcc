#!/bin/sh
lcov -z -d bld/CMakeFiles/dcc_original.dir/src
./regression_tester.rb bld/dcc_original -a2 -m -s -c -V 2>stderr >stdout
lcov -c -d bld/CMakeFiles/dcc_original.dir/src -o cover1.info
lcov -e cover1.info *dcc_* -o cover2.info
genhtml -o coverage -f --demangle-cpp cover2.info

