require "#{File.dirname(__FILE__)}/helper"
require 'test/unit'

class TestExpectedResults < Test::Unit::TestCase
  def test_expected_results
    expected_results = get_expected_barzer_results
    current_results = get_current_barzer_results
    
    current_results["test"].each do |t|
      query = t["query"][0]
      current_barz = t["barz"]
      expected_barz = begin
        expected_results["test"].find { |x| x["query"][0] == query }["barz"]
      rescue NoMethodError
        nil
      end
      assert_equal expected_barz, current_barz, "checking barz for '#{query}'"
    end
  end
end
