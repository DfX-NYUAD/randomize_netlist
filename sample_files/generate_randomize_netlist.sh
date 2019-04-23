#!/bin/bash

if [ $# -lt 2 ]; then
	echo "Please provide the following parameters:"
	echo "	1) Folder with netlists to work on"
       	echo "	2) Run ID"
        echo "	3) Optional; fully specified commands, as one parameter, e.g., \"--random_op=1\""
	echo ""
	exit
fi

folder=$1
run_ID=$2

# reading in values for optional parameters, if given
if [ "$3" != "" ]; then
	commands=$3
else
	commands=""
fi

# cleanup old sbatch.sh, if any
rm sbatch.sh

# link relevant files, in case they are not available already
ln -s /home/jk176/randomize_netlist/Randomize_Netlist .
ln -s /home/jk176/randomize_netlist/sample_files/randomize_netlist.sh.template .
ln -s /home/jk176/randomize_netlist/sample_files/cells.functions .
ln -s /home/jk176/randomize_netlist/sample_files/cells.inputs .
ln -s /home/jk176/randomize_netlist/sample_files/cells.outputs .

for file in `ls $folder/*.v`
do
	name=$(basename $file .v)
	run="run_"$run_ID
	script="randomize_"$name"_"$run".sh"

	echo "Generate $script ..."

	cp randomize_netlist.sh.template $script

	sed_string="s,netlist=TODO,netlist=$file,g"
	sed -i "$sed_string" $script

	sed_string="s,golden_netlist=TODO,golden_netlist=$file,g"
	sed -i "$sed_string" $script

	sed_string="s,SBATCH -o TODO,SBATCH -o "$name"_"$run".log,g"
	sed -i "$sed_string" $script

	sed_string="s,SBATCH -e TODO,SBATCH -e "$name"_"$run".log.slurm,g"
	sed -i "$sed_string" $script

	sed_string="s,commands=TODO,commands=\""$commands\"",g"
	sed -i "$sed_string" $script

	echo "sbatch $script" >> sbatch.sh
done
