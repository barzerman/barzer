#!/bin/ksh

NUMITER=200
if [[ -n $1 ]]; then
	NUMITER=$1
fi

blast() {
integer i 
i=0
while [[ i -lt $NUMITER ]]; do
echo "<query>pitbulls last monday under 200 dollars $i</query>
.
" | nc localhost 5666
((i=i+1))
done
}

integer k
k=0
while [[ k -lt 5 ]]; do
	blast &
	((k=k+1))
done

wait
