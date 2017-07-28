#!/bin/sh
# Usage: sh this-file.sh tool-dir input-dir output-folder-name
while read LINE
do
	TEST_CASE=`echo -n $LINE | tail -c 4`

	# Getting line and column number of EOF
	LAST_LINE_NUMBER=`grep -c ^ $2`
	LAST_LINE="`tail -1 $2`"
	LAST_LINE_COL_NUMBER=$((1+`expr length $LAST_LINE`))

	echo "$1 $2 -o ${3}/option-m/$TEST_CASE -l 10 -rs 1 1 -re $LAST_LINE_NUMBER $LAST_LINE_COL_NUMBER $LINE"

	$1 $2 -o ${3}/option-m/$TEST_CASE -l 10 -rs 1 1 -re $LAST_LINE_NUMBER \
	$LAST_LINE_COL_NUMBER $LINE > /dev/null

	if test $? != 0; then
		echo "Error"
	fi
done < option-m-all-op.txt