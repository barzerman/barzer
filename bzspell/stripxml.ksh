#!/bin/ksh

while getopts f: name
   do
     case $name in
       f) OUTFILE=$OPTARG;;
       ?) exit 2;; 
     esac
done
shift $(($OPTIND - 1))

if [[ -z $OUTFILE ]]; then 
	OUTFILE=$(basename $0).out
	echo "use -f to specify output file name" >&2
fi
echo "output file name set to $OUTFILE" >&2

sed -e 's/<t>/|/g' -e 's/<\/t>/|/g' $* | grep '|' | awk -F'|' '{ for (i=1; i<=NF; ++i ) if( length($i)>3 ) print $i }' |\
egrep -v '[^a-z]' | sort | uniq -c  | sort -n > ${OUTFILE}

if [[ -s ${OUTFILE}.out ]]; then 
	echo "output is in ${OUTFILE}" >&2 
else 
	echo "did not produce valid ${OUTFILE}"
fi
