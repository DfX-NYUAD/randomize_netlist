#!/bin/bash

if [ $# -lt 1 ]; then
	echo "Please provide the following parameters:"
	echo "	1) netlist file to fix"
	echo ""
	echo "Note that the file will be backed up to file.back before editing it"
	echo ""
	exit
fi

file=$1

cp $file $file.back

sed 's,\\,_,g' $file | sed 's,\/,_,g' | sed 's,\[,_,g' | sed 's,\],_,g' > $file.fixed

mv $file.fixed $file
