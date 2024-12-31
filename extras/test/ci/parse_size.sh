#!/bin/bash

board=$1
hash=$(git rev-parse --short HEAD)

in_file="size_$board.txt"
out_file="size_${hash}_${board//:/-}.csv"
rm -f $out_file

# write the header
echo "text,data,bss,dec,hex,filename" > "$out_file"

# convert to CSV
awk 'NR > 1 {print $7 "," $8 "," $9 "," $10 "," $11 "," $12}' "$in_file" >> "$out_file"

# remove input file
rm -f $in_file

