#! /bin/bash

file=cppcheck.txt
cppcheck --version
cppcheck src --enable=all \
             --force \
             --inline-suppr \
             --suppress=ConfigurationNotChecked \
             --suppress=unusedFunction \
             --suppress=missingIncludeSystem \
             --suppress=missingInclude \
             --quiet >> $file 2>&1
echo "Cppcheck finished with exit code $?"

error=$(grep ": error:" $file | wc -l)
warning=$(grep ": warning:" $file | wc -l)
style=$(grep ": style:" $file | wc -l)
performance=$(grep ": performance:" $file | wc -l)
echo "found $error erros, $warning warnings, $style style and $performance performance issues"
if [ $error -gt "0" ] || [ $warning -gt "0" ] || [ $style -gt "0" ] || [ $performance -gt "0" ]
then
  cat $file
  exitcode=1
fi

rm $file
exit $exitcode
