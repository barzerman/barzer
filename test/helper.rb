require 'rubygems'
require 'xmlsimple'
require 'erb'

QUERIES_FILENAME = 'data/queries.txt'
EXPECTED_RESULTS_FILENAME = "data/expected_results.xml"

def with_cwd(dir)
  original_dir = Dir.pwd
  begin
    Dir.chdir dir
    yield
  ensure
    Dir.chdir original_dir
  end
end

def run_barzer(params)
  with_cwd('..') { `./barzer.exe #{params}` }
end

def get_expected_barzer_results
  f = File.new(EXPECTED_RESULTS_FILENAME)
  contents = ERB.new(f.readlines.join).result
  f.close
  XmlSimple.xml_in(contents)
end

def get_current_barzer_results
  output = run_barzer("test -i test/#{QUERIES_FILENAME}")
  XmlSimple.xml_in(output)
end
