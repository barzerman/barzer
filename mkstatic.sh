#!/bin/bash
set -x

BINDIR=$(dirname $0)
DISTROFILE=${BINDIR}/linuxdistrofiles.txt
CURDIR=$(pwd)
OUTDIR=dist/barzer
OUTFILE=${CURDIR}/barzer.linux${VERSION}.tar.bz2

CMD="cmake -DCMAKE_BUILD_TYPE=Release -DSTATIC_BUILD=ON -DINSTALL_DIR=$CURDIR/$OUTDIR $(dirname $0)" 
echo $CMD
eval $CMD
make -j7
VERSION=""
if [[ -n $1 ]]; then
    VERSION=$1
fi



echo "making ${OUTFILE} in ${OUTDIR}"

rm -rf ${OUTDIR}
mkdir -p ${OUTDIR}

make install

strip $OUTDIR/*.exe

tar cjf $OUTFILE $OUTDIR

echo "done"
