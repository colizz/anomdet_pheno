if [[ -z $1 ]]; then
    echo "Usage: ./merger.sh <dataset>"
    return
fi

dir_path=/data/bond/licq/datasets/JetClassII/$1
mkdir -p ${dir_path}_merged

# Counter
count=0
index=0
A=""

# Find all files, sort them, and iterate
find "$dir_path" -type f | sort | while IFS= read -r file; do
    A+="$file "

    # Increment the counter
    ((count++))

    # Every 10 files, print a newline
    if ((count % 10 == 0)); then
        hadd ${dir_path}_merged/ntuples_$index.root $A
        ((index++))
        A=""
    fi
done
hadd ${dir_path}_merged/ntuples_$index.root $A
