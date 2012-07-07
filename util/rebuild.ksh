#!/bin/ksh

PULL=yes
GITBRANCH=master
while getopts "dDb:" opt
do
case $opt in
d) DEPLOY="yes";; ## -d pull/make/install
D) unset PULL; DEPLOY=yes;; ## -D install only
b) unset PULL; DEPLOY=yes;; ## -D install only
esac
done
shift $((OPTIND-1))

TIMESTAMP=$(date +%y%m%d-%H:%M:%S)
LOGFILE=/home/barzer/src/logs/$(basename $0).log.${TIMESTAMP}
LOCK=rebuild.lock
if [[ -f $LOCK ]]; then
    {
        echo $LOCK detected ... aborting
    } >&2 
else
    touch $LOCK
    echo created ${LOCK}
fi

{
    {
    cd /home/barzer/src/barzer
    if [[ -n $PULL ]]; then
        echo "REBUILD:	$(date +%y/%m/%d-%H:%M:%S)	git pull"
        git pull origin ${GITBRANCH}
    else
        echo "REBUILD:	SKIPPED git pull"
    fi
    #chmod -R og-xr barzer

    echo "REBUILD:	$(date +%y/%m/%d-%H:%M:%S)	cmake"
    cmake . -DCMAKE_BUILD_TYPE="Release"
    echo "REBUILD:	$(date +%y/%m/%d-%H:%M:%S)	make"
    make -j4
    echo "REBUILD:	$(date +%y/%m/%d-%H:%M:%S)	make finished"

    if [[ -n $DEPLOY ]]; then
        cd /home/barzer/src/barzer
        echo "REBUILD:	$(date +%y/%m/%d-%H:%M:%S)	install"
        make install
        echo "REBUILD:	$(date +%y/%m/%d-%H:%M:%S)	install finished"
    else
        echo "REBUILD:	skipping install (run with -D to install or -d to pull and install)"
    fi
    } > ${LOGFILE} 2>&1
} &
echo results written to ${LOGFILE} 
{
    tail -f ${LOGFILE} | grep "^REBUILD"
} &
wait %1
kill %2
echo removing $LOCK
rm $LOCK
