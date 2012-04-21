#!/bin/ksh

#make sure you ran make OPT -O3 
#make sure you stripped barzer.exe pybarzer.so and libbarzer.so are stripped 

for i in $(<distro/shareesah.pack.txt)
do

CURDIR=$(pwd)
OUTDIR=distro/linux

mkdir -p ${OUTDIR}
cp $i ${OUTDIR}
done 
cd ${OUTDIR}

OUTFILE=${CURDIR}/${OUTFILE}.barzer.linux
tar cvf - * | gzip -c > ${OUTFILE}

ls -l ${OUTFILE}

