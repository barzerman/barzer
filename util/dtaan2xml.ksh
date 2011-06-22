#!/bin/ksh

sed -e 's/&/\&amp;/g' -e 's/"/\&quot;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' | ./deumlaut | gawk -f dtaan2xml.awk -F'|' 
