#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <path to check>"
    exit 1
fi

path=$1

cppcheck --version
cppcheck $path --enable=all \
             --force \
             --inline-suppr \
             --suppress=ConfigurationNotChecked \
             --suppress=unusedFunction \
             --suppress=missingIncludeSystem \
             --suppress=missingInclude \
             --quiet
