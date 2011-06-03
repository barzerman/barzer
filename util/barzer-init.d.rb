#!/usr/bin/env ruby

# init.d startup script for multiple barzer instances

require 'rubygems'
require 'xmlsimple'
require 'fileutils'

INSTANCES_CONFIG = '/etc/barzer/barzer-instances.xml'
PID_DIR = '/var/run'
LOG_DIR = '/var/log'
BARZER_USER = 'barzer'
BARZER_DIR = '/usr/share/barzer'
CONFIG_DIR = "#{BARZER_DIR}/data/configs"
BARZER_BIN = 'barzer.exe'
DAEMON_BIN = '/usr/bin/daemon'
SU_BIN = '/bin/su'

def read_instances_config(config_file)
  data = XmlSimple.xml_in(config_file)
  data['instance']
end

def usage
  puts "Usage: #{File.basename(__FILE__)} {start|stop|status|restart}"
end

def daemon_running?(instance)
  system("#{DAEMON_BIN} #{daemon_args(instance)} --running")
end

def pid_file(i)
  "#{PID_DIR}/barzer-#{i['name']}.pid"
end

def get_pid(i)
  f = File.open(pid_file(i))
  pid = f.readline.to_i
  f.close
  pid
end

def daemon_args(i)
  ["--name=barzer-#{i['name']}",
   "--inherit",
   "--chdir=#{BARZER_DIR}", 
   "--output=#{LOG_DIR}/barzer-#{i['name']}.log",
   "--pidfile=#{pid_file(i)}"].join(' ')
end

def start_instance(i)
  pidfile = pid_file(i)

  # Making sure pid file is writable by barzer user
  FileUtils.touch(pidfile) unless File.exist?(pidfile)
  FileUtils.chown(BARZER_USER, nil, pidfile)
  
  if daemon_running?(i)
    puts "Instance #{i['name']} is already running."
  else
    barzer_cmd = "#{BARZER_DIR}/#{BARZER_BIN} server #{i['port']} -cfg #{CONFIG_DIR}/#{i['config']}.xml"
    start_cmd = "#{SU_BIN} -l #{BARZER_USER} --shell=/bin/bash -c \"#{DAEMON_BIN} #{daemon_args(i)} -- #{barzer_cmd}\""
    start_success = system(start_cmd)
    if start_success
      puts "Instance #{i['name']} started successfully."
      return true
    else
      puts "Instance #{i['name']} failed to start."
      return false
    end
  end
end

def start(instances)
  puts "Starting barzer instances..."

  instances.each { |i| start_instance(i) }
end

def processes_running?
  `ps -U #{BARZER_USER} --no-headers -f|egrep -e '(barzer|daemon)'|wc -l`.to_i > 0
end

def force_stop
  system("killall -u #{BARZER_USER} #{BARZER_BIN} daemon") if processes_running?
end

def stop_instance(i)
  if daemon_running?(i)
    system("#{DAEMON_BIN} #{daemon_args(i)} --stop")

    tries = 0
    while daemon_running?(i) && tries < 5 do
      tries += 1
    end

    return tries < 5
  else
    return true
  end
end

def stop(instances)
  puts "Stopping barzer instances..."

  stop_failures = []
  instances.each { |i| stop_failures << i['name'] unless stop_instance(i) }
  force_stop if stop_failures

  # Many daemons don't delete their pidfiles when they exit.
  instances.each { |i| File.delete(pid_file(i)) if File.exist?(pid_file(i)) }
end

def status(instances)
  instances.each do |i|
    if daemon_running?(i)
      puts "Instance #{i['name']} is running with the pid #{get_pid(i)}"
    else
      # TODO: detect stray processes and stray pid files
    end
  end
end

def restart(instances)
  puts "Restarting barzer instances..."
  stop instances
  start instances
end

def main
  instances = read_instances_config(INSTANCES_CONFIG)
  
  if (ARGV.length < 1)
    usage
  else
    case ARGV[0]
    when "start"
      start instances
    when "stop"
      stop instances
    when "status"
      status instances
    when "restart"
      restart instances
    else
      usage
    end
  end
end

main
