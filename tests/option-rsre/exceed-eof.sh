#!/bin/sh
# COMUT exits on error in which user provides a location exceeding EOF
# This test also represents the case user provides a location exceeding end of line

if test $# = 0; then
    echo "Usage: sh filename.sh executable-COMUT"
    echo "Error: no executable-COMUT file was given"
    exit 1
fi

test_exceed_eof()
{
	mkdir $OUTPUT_FOLDER_NAME   

    echo "$3 input-src/${TEST_INPUT} -o $OUTPUT_FOLDER_NAME $1 $2"

    # Run the tool with the input source and start of mutation range exceeding EOF
    $3 input-src/${TEST_INPUT} -o $OUTPUT_FOLDER_NAME $1 $2 > /dev/null 2>&1

    # The test success if exit value is NOT 0
    # and no files are generated in output folder
    if test $? != 0 && test `find ${OUTPUT_FOLDER_NAME} -type f -name \* | wc -l` = 0
    then
        echo "[SUCCESS] $TEST_INPUT $1 exceeds EOF"
    else
        echo "[FAIL] $TEST_INPUT $1 exceeds EOF"
    fi
    
    # Remove created output folder for this input source file
    rm -R $OUTPUT_FOLDER_NAME
}

# DIR: the directory that this script is in
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Executing $0"
cd input-src

# Exit script if there is no input source file
if test `find . -type f -name \* | wc -l` = 0 ; then
    exit
fi

for TEST_INPUT in *
do
    cd $DIR

    # Getting line and column number of EOF
    LAST_LINE_NUMBER=`grep -c ^ input-src/$TEST_INPUT`
    LAST_LINE="`tail -1 input-src/$TEST_INPUT`"
    LAST_LINE_COL_NUMBER=$((1+`expr length $LAST_LINE`)) 

    # make folder name in output directory with input source name without .c
    OUTPUT_FOLDER_NAME=`echo "output/$TEST_INPUT" | sed 's/.\{2\}$//'`

    test_exceed_eof "-rs" "$LAST_LINE_NUMBER $((3+LAST_LINE_COL_NUMBER))" $1 
    test_exceed_eof "-rs" "$((1+$LAST_LINE_NUMBER)) $((3+LAST_LINE_COL_NUMBER))" $1
    test_exceed_eof "-re" "$LAST_LINE_NUMBER $((3+LAST_LINE_COL_NUMBER))" $1
    test_exceed_eof "-re" "$((1+$LAST_LINE_NUMBER)) $((3+LAST_LINE_COL_NUMBER))" $1 

    cd input-src
done