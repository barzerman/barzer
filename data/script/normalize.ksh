#!/bin/ksh

#normalize 
#tr "[:upper:]" "[:lower:]" | sed -e 's/w\//with /g' -e 's/  */ /g' -e 's/[_"\/]/ /g' -e 's/\([glbfwptnmrdk]\)s\b*/\1/g' -e 's/\([glbfwptnmrdkc]e\)s\b*/\1/g'  \
#-e 's/ | /|/g' -e 's/[\\*]//g' -e 's/^ *//g'
tr "[:upper:]" "[:lower:]" | sed -e 's/w\//with /g'  -e 's/[_"\/]/ /g' \
-e "s/'s / /g" \
-e 's/&/ and /g' \
-e 's/\([a-z][a-z][cnmptorvfls]\)es /\1e /g'  \
-e 's/\([a-z][a-z][cnmptorvfls]\)es$/\1e/g'  \
-e 's/tches /tch/g'  \
-e 's/\([a-z][a-z][glbfwptnmrdk]\)s /\1 /g' \
-e 's/\([a-z][a-z][glbfwptnmrdk]\)s$/\1/g' \
-e 's/ | /|/g' -e 's/[\\*]//g' -e 's/^ *//g'  -e 's/  */ /g'
echo " done "

