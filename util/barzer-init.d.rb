#!/usr/bin/env ruby

# init.d startup script for multiple barzer instances

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
SU_BIN = '/usr/bin/su'

def read_instances_config(config_file)
  data = XmlSimple.xml_in(config_file)
  data['instance']
end

def usage
  puts "Usage: #{File.basename(__FILE__)} {start|stop|status|restart}"
end

def start(instances)
  puts "Starting barzer instances..."

  instances.each do |i|
    
    pidfile = "#{PID_DIR}/barzer-#{i['name']}.pid"
    logfile = "#{LOG_DIR}/barzer-#{i['name']}.log"

    # Making sure pid file is writable by barzer user
    FileUtils.touch(pidfile) unless File.exist?(pidfile)
    FileUtils.chown(BARZER_USER, pidfile)

    daemon_args = ["--name=barzer-#{i['name']}",
                   "--inherit",
                   "--chdir=#{BARZER_DIR}", 
                   "--output=#{logfile}",
                   "--pidfile=#{pidfile}"].join(' ')

    is_running = system("#{DAEMON_BIN} #{daemon_args} --runninig")

    if is_running
      puts "Instance #{i['name']} is already running."
    else
      barzer_cmd = "#{BARZER_BIN} server #{i['port']} -cfg #{CONFIG_DIR}/#{i['config'].xml"
      start_success = system("#{SU_BIN} -l #{BARZER_USER} --shell=/bin/bash -c \"#{DAEMON_BIN} #{daemon_args} -- #{barzer_cmd}\"")
      if start_success
        puts "Instance #{i['name']} started successfully."
      else
        puts "Instance #{i['name']} failed to start."
      end
    end
  end
  
end

def processes_running?
  `ps -U #{BARZER_USER} --no-headers -f|egrep -e '(barzer|daemon)'|wc -l`.to_i > 0
end

def force_stop
  system("killall -u #{BARZER_USER} #{BARZER_BIN} daemon") if processes_running?
end

def stop(instances)
  puts "Stopping barzer instances..."
end

def status(instances)
end

def restart(instances)
  puts "Restarting barzer instances..."
end

def main
  instances = read_instances_config(INSTANCES_CONFIG)
  
  if (ARGV.length < 1)
    usage
  else
    case ARGV[0]
    when "start"
      start
    when "stop"
      stop
    when "status"
      status
    when "restart"
      restart
    else
      usage
    end
  end
end

main
