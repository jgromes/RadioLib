#! /bin/bash

script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
out_file=$script_dir/uncrustify.patch
rm $out_file

# for all soruce files, run uncrustify and save the output to a patch file
for file in $(find src/ -name '*.cpp' -or -name '*.h'); do 
    uncrustify -c $script_dir/uncrustify.cfg -f $file -o $file.uncrustify
    diff -u $file $file.uncrustify >> $out_file
    rm $file.uncrustify
done

cat $out_file

if [ -s $out_file ]; then
    echo "Uncrustify finished and found some issues"
    echo "Apply the patch file by: 'cd src && git apply $out_file'"
    exit 1
else
    echo "Uncrustify finished - all OK!"
    exit 0
fi

