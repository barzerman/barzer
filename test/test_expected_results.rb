require "#{File.dirname(__FILE__)}/helper"
require 'test/unit'

class TestExpectedResults < Test::Unit::TestCase
    def test_expected_results
    expected_results = get_expected_barzer_results
    current_results = get_current_barzer_results
    
    expected_results.each_pair do |query, result|
      assert result == current_results[query]
    end
  end
end
