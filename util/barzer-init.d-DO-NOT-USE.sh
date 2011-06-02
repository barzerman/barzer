#!/bin/bash

# An old startup script, deals with just one instance.
# Replaced by barzer-init.d.rb

PATH=/bin:/usr/bin:/sbin:/usr/sbin

DESC="Barzer Server"
NAME=barzer
SCRIPTNAME=/etc/init.d/$NAME

BARZER_USER=barzer
BARZER_LOG=/var/log/barzer.log
PIDFILE=/var/run/barzer.pid
BARZER_DIR=/usr/share/barzer
BARZER_BIN=barzer.exe
BARZER_PORT=1488

DAEMON=/usr/bin/daemon
DAEMON_ARGS="--name=$NAME --inherit --chdir=$BARZER_DIR --output=$BARZER_LOG --pidfile=$PIDFILE" 

SU=/bin/su

. /lib/lsb/init-functions

if [ `id -u` -ne 0 ]; then
    echo "The $NAME init script can only be run as root"
    exit 1
fi

do_start()
{
    mkdir `dirname $PIDFILE` > /dev/null 2>&1 || true
    chown $BARZER_USER `dirname $PIDFILE`

    # Return
    #   0 if daemon has been started
    #   1 if daemon was already running
    #   2 if daemon could not be started
    $DAEMON $DAEMON_ARGS --running && return 1

    # --user in daemon doesn't prepare environment variables like HOME, USER, LOGNAME or USERNAME,
    # so we let su do so for us now
    $SU -l $BARZER_USER --shell=/bin/bash -c "$DAEMON $DAEMON_ARGS -- $BARZER_DIR/$BARZER_BIN server $BARZER_PORT" || return 2
}


#
# Verify that all barzer processes have been shutdown
# and if not, then do killall for them
# 
get_running() 
{
    return `ps -U $BARZER_USER --no-headers -f | egrep -e '(barzer|daemon)' | grep -c . `
}

force_stop() 
{
    get_running
    if [ $? -ne 0 ]; then 
	killall -u $BARZER_USER $BARZER_BIN daemon || return 3
    fi
}

# Get the status of the daemon process
get_daemon_status()
{
    $DAEMON $DAEMON_ARGS --running || return 1
}


#
# Function that stops the daemon/service
#
do_stop()
{
    # Return
    #   0 if daemon has been stopped
    #   1 if daemon was already stopped
    #   2 if daemon could not be stopped
    #   other if a failure occurred
    get_daemon_status 
    case "$?" in
	0) 
	    $DAEMON $DAEMON_ARGS --stop || return 2
        # wait for the process to really terminate
        for n in 1 2 3 4 5; do
            sleep 1
            $DAEMON $DAEMON_ARGS --running || break
        done
        if get_daemon_status; then
	        force_stop || return 3
        fi
	    ;;
	*)
	    force_stop || return 3
	    ;;
    esac

    # Many daemons don't delete their pidfiles when they exit.
    rm -f $PIDFILE
    return 0
}

case "$1" in
  start)
    log_daemon_msg "Starting $DESC" "$NAME"
    do_start
    case "$?" in
        0|1) log_end_msg 0 ;;
        2) log_end_msg 1 ;;
    esac
    ;;
  stop)
    log_daemon_msg "Stopping $DESC" "$NAME"
    do_stop
    case "$?" in
        0|1) log_end_msg 0 ;;
        2) log_end_msg 1 ;;
    esac
    ;;
  restart|force-reload)
    #
    # If the "reload" option is implemented then remove the
    # 'force-reload' alias
    #
    log_daemon_msg "Restarting $DESC" "$NAME"
    do_stop
    case "$?" in
      0|1)
        do_start
        case "$?" in
          0) log_end_msg 0 ;;
          1) log_end_msg 1 ;; # Old process is still running
          *) log_end_msg 1 ;; # Failed to start
        esac
        ;;
      *)
  	# Failed to stop
	log_end_msg 1
	;;
    esac
    ;;
  status)
      get_daemon_status
      case "$?" in 
	 0) echo "$DESC is running with the pid `cat $PIDFILE`";;
         *) 
	      get_running
	      procs=$?
	      if [ $procs -eq 0 ]; then 
		  echo -n "$DESC is not running"
		  if [ -f $PIDFILE ]; then 
		      echo ", but the pidfile ($PIDFILE) still exists"
		  else 
		      echo
		  fi

	      else 
		  echo "$procs instances of jenkins are running at the moment"
		  echo "but the pidfile $PIDFILE is missing"
	      fi
	      ;;
      esac
    ;;
  *)
    echo "Usage: $SCRIPTNAME {start|stop|status|restart|force-reload}" >&2
    exit 3
    ;;
esac

exit 0
