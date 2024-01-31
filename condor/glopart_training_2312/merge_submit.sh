#!/bin/bash

dir_path=$1
N=$2

if [[ -z $N ]]; then
    N=10
fi

# Check if N is a valid number
if ! [[ "$N" =~ ^[0-9]+$ ]]; then
    echo "Error: N must be a positive integer."
    exit 1
fi

# Find, sort, and process files
count=0
group=""
output=()
while IFS= read -r file; do
    group+="${file},"
    ((++count % N == 0)) && { output+=("${group%,}"); group=""; }
done < <(find "$dir_path" -maxdepth 1 -type f -name "*.root" | sort -V)

# Add last group if it exists
[[ -n $group ]] && output+=("${group%,}")

# Print all groups
printf "%s\n" "${output[@]}"

# Iterating each groups to submit a file
for i in "${!output[@]}"; do
    hep_sub merge_hadd.sh -argu ${dir_path%/}_merged/ntuples_$i.root ${output[$i]}
done
