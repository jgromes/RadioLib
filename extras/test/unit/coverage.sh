#!/bin/bash

set -e
filename="lcov"
rm -rf $filename.*
lcov --capture --directory build --output-file "${filename}.info" --gcov-tool gcov-11

# filter out boost and C++ standard library
lcov --remove "${filename}.info" "/usr/*/boost/*" "/usr/include/c++/*" --output-file "${filename}.info"

# generate HTML
genhtml "${filename}.info" --output-directory "${filename}.report"
zip -r "${filename}.report.zip" "${filename}.report"
