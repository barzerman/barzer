#!/bin/ksh


CURDIR=$(pwd)
BINDIR=$(dirname $0)
DISTROFILE=${BINDIR}/linuxdistrofiles.txt
OUTDIR=barzer
OUTFILE=${CURDIR}/barzer.mac${VERSION}.tar.bz2
BUILDDIR=macbuild
BINFILELIST=binfilelist.out

if [[ ! -d  $BUILDDIR ]]; then
    mkdir -p $BUILDDIR
fi 

cd $BUILDDIR

echo "cmake .. -DCMAKE_BUILD_TYPE=Release"
cmake .. -DCMAKE_BUILD_TYPE="Release"
make -j5

VERSION=""
if [[ -n $1 ]]; then
    VERSION=$1
fi


echo "making ${OUTFILE} in ${OUTDIR}"

rm -rf ${OUTDIR}
mkdir -p ${OUTDIR}

for i in $(<$DISTROFILE)
do
    if [[ $i = *.so || $i = *.exe || $i = *.dylib]]; then
        echo "processing $i" >&2
        ${BINDIR}/getdeplibs.py $i
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

do_subs()
{
   for lname in `otool -L $1 | tail -n +2 | awk '{print $1}' | grep "^/"| grep -vE "$1|@"`; do
        LOCALNAME=$(basename $lname)
        if [[ -f $LOCALNAME ]]; then
            echo "$1: $lname -> @$2/$LOCALNAME"
            install_name_tool -change $lname "@$2/$LOCALNAME" $1
        fi
    done
}

for fname in *.dylib *.so
do
    do_subs $fname "loader_path"
    install_name_tool -id "@loader_path/$fname" $fname
done

do_subs "barzer.exe" "executable_path"


if [[ $? -eq 0 ]]; then
    echo "*** creating archive ===> ${OUTFILE}"
    cd -
    tar cjvf ${OUTFILE} ${OUTDIR}


    echo "archive created from files in ${OUTDIR}"
    ls -l ${OUTFILE}
else
    echo "failed to cd to ${OUTDIR}"
    exit 1
fi
echo "to deploy  copy to yanis@barzer.net:/home/barzer/public_html/tranio/"
