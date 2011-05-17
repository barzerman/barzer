require 'xmlsimple'

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
  output = with_cwd('..') { `./barzer.exe #{params}` }
  XmlSimple.xml_in(output)
end

def get_expected_barzer_results
  XmlSimple.xml_in(EXPECTED_RESULTS_FILENAME)
end

def get_current_barzer_results
  run_barzer "test -i test/data/queries.txt"
end
