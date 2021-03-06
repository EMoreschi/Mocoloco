#!/bin/bash

g++ -o test ../MocoLoco.cpp;

Jaspar="../Jaspar_2020/MA0502.2.jaspar ../Jaspar_2020/MA0002.2.jaspar ../Jaspar_2020/MA0042.2.jaspar ../Jaspar_2020/MA0071.1.jaspar ../Jaspar_2020/MA0093.3.jaspar"
Twobit="../Genomes/hg38/hg38.2bit ../Genomes/hg19/hg19.2bit"
BED="../Test_Bed/baitedregionsb38.bed ../Test_Bed/nfy_k562_hg19.bed"

for a in $BED 
do
	for b in $Twobit 
	do
		for c in $Jaspar 
		do
			./test -b ${a} -t ${b} -j ${c};
			
		done
	done
done
