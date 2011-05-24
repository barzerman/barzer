#!/usr/bin/env ruby

# Restarts barzer daemon if the installed version is newer than the currently running one.

PID_FILE = '/var/run/barzer.pid'
EXE_FILE = '/usr/share/barzer/barzer.exe'

pid_m = File.mtime(PID_FILE)
exe_m = File.mtime(EXE_FILE)

system("service", "barzer", "restart") if (pid_m < exe_m)
