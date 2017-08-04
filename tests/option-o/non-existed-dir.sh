#!/bin/sh
# COMUT exit on error that user provides non-existed directory

if test $# = 0; then
    echo "Usage: sh filename.sh executable-COMUT"
    echo "Error: no executable-COMUT file was given"
    exit 1
fi

# DIR: the directory that this script exist in
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Executing $0"
cd input-src

for t in *
do
    cd $DIR

    # make folder name in output directory with input source name without .c
    OUTPUT_FOLDER_NAME=`echo "output/$t" | sed 's/.\{2\}$//'`
    mkdir $OUTPUT_FOLDER_NAME
    
    # Run the tool with the input source and a non-existed directory
    $1 input-src/${t} -o output/non-existed-dir > /dev/null 2>&1

    # The test success if exit value is NOT 0
    # and no files are generated in output folder
    if test $? != 0 && test `find ${OUTPUT_FOLDER_NAME} -type f -name \* | wc -l` = 0
    then
        echo "[SUCCESS] $t directory does not exist"
    else
        echo "[FAIL] $t directory does not exist"
    fi

    # Remove created output folder for this input source file
    rm -R $OUTPUT_FOLDER_NAME

    cd input-src
done
