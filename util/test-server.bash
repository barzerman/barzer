#!/bin/bash

PORT=5666
while getopts p:e: name
   do
     case $name in
       p) PORT=$OPTARG;;
       e) RULEFILE=$OPTARG;;
       ?) exit 2;;
     esac
done
shift $(($OPTIND - 1))

exec 3<> /dev/tcp/localhost/$PORT 

if [[ -z $RULEFILE ]]; then
	QUERY=$*
	echo -e "<query>${QUERY}</query>\r\n.\r\n" >&3
	cat <&3
else
    echo -e "!!EMIT:$(cat ${RULEFILE})\r\n.\r\n" >&3
    cat <&3
fi

exec 3<&-