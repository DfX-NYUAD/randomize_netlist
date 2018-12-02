#!/bin/bash

for file in `ls input/`
do
	name=${file%.v}
	script="randomize_"$name".sh"

	echo "Generate $script ..."

	cp randomize_netlist.sh.template $script

	sed_string="s,netlist=TODO,netlist=input/$file,g"
	sed -i "$sed_string" $script

	sed_string="s,golden_netlist=TODO,golden_netlist=input/$file,g"
	sed -i "$sed_string" $script

	sed_string="s,SBATCH -o TODO,SBATCH -o "$name".log,g"
	sed -i "$sed_string" $script

	sed_string="s,SBATCH -e TODO,SBATCH -e "$name".log.slurm,g"
	sed -i "$sed_string" $script
done
