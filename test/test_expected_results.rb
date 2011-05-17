require "#{File.dirname(__FILE__)}/helper"
require 'test/unit'

class TestExpectedResults < Test::Unit::TestCase
    def test_expected_results
    expected_results = get_expected_barzer_results
    current_results = get_current_barzer_results
    
    expected_results["test"].each do |t|
      query = t["query"][0]
      expected_barz = t["barz"]
      current_barz = current_results["test"].find { |x| x["query"][0] == query }["barz"]
      assert_equal current_barz, expected_barz, "checking barz for '#{query}'"
    end
  end
end
