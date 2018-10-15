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

# replace special characters
# the 3rd sed command is to restore "//" comments
sed 's,\\,__,g' $file | sed 's,\/,__,g' | sed 's,____,//,g' | sed 's,\[,__,g' | sed 's,\],__,g' > $file.fixed

mv $file.fixed $file
