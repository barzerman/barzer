#!/bin/ksh

cmake . -DCMAKE_BUILD_TYPE="Release"
make -j5
VERSION=""
if [[ -n $1 ]]; then
    VERSION=$1
fi

DISTROFILE=linuxdistrofiles.txt
CURDIR=$(pwd)
OUTDIR=distro/linux
OUTFILE=${CURDIR}/barzer.linux${VERSION}.tar.bz2

BINFILELIST=binfilelist.out
echo "making ${OUTFILE} in ${OUTDIR}"

rm -rf ${OUTDIR}
mkdir -p ${OUTDIR}

for i in $(<$DISTROFILE)
do
    if [[ $i = *.so || $i = *.exe ]]; then
        echo "processing $i" >&2
        ./getdeplibs.py $i
        strip $i
    fi
done | sort -u > ${BINFILELIST}
{
    echo binary files to copy are in ${BINFILELIST}
    #cat ${BINFILELIST}
} >&2

echo "*** copying files to ===> ${OUTDIR}"
cp $(<$DISTROFILE) $(<$BINFILELIST) ${OUTDIR}
cd ${OUTDIR}
if [[ $? -eq 0 ]]; then
echo "*** creating archive ===> ${OUTFILE}"
tar cjvf ${OUTFILE} *
#cd ${OUTDIR}

#tar cvf - * | gzip -c > ${OUTFILE}

echo "archive created from files in ${OUTDIR}"
ls -l ${OUTFILE}
else
    echo "failed to cd to ${OUTDIR}"
    exit 1
fi
# copy to yanis@barzer.net:/home/barzer/public_html/tranio/
