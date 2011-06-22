#!/bin/ksh

OUTDIR=data/
while getopts f:d: name
   do
     case $name in
       f) OUTFILE=$OPTARG;;
       d) OUTDIR=$OPTARG;;
       ?) exit 2;;
     esac
done
shift $(($OPTIND - 1))
OUTDIRTMP=${OUTDIR}/out/
if [[ -z $OUTFILE ]]; then 
	echo "usage $(basename $0) -f outfile file1 ..."
	exit 1 
else
	echo results to be stored in $OUTFILE
fi
OUTFILETMP=${OUTDIRTMP}/${OUTFILE}.tmp

XMLFILES=$*
mkdir -p ${OUTDIRTMP}

BINDIR=$(dirname $0)
# creating subs xml files 
echo  "creating subset files "
dir=${OUTDIRTMP}
for f in $XMLFILES; do
	fn=$(basename $f)
	print -n "$f ---> ${dir}/$fn ... "
	sed -e 's/<pat>/<pat><subset>/g' -e 's/<\/pat>/<\/subset><\/pat>/g' $f | ${BINDIR}/deumlaut > ${dir}/$fn
	echo " done $(wc -l ${dir}/$fn| awk '{ print $1}') lines" 
done

echo "analyzing data set ... loading into barzer"
./barzer.exe shell -anlt -cfg data/configs/astor_auto.xml <<-EOF
dtaan ${OUTFILETMP}
exit
EOF

echo "converting $OUTFILE.txt to xml"
sed -e 's/&/\&amp;/g' -e 's/"/\&quot;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' ${OUTFILETMP} | \
gawk -f ${BINDIR}/dtaan2xml.awk -F'|' >  ${OUTDIR}/${OUTFILE}.xml

echo "done : produced xml file ${OUTDIR}/${OUTFILE}.xml"
