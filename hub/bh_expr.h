#pragma once
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/function.hpp>


namespace bh {
const int test = 5;
typedef std::vector<double> params;


struct Context {
    params values;
    Context(params& ps) : values(ps) {}
};

typedef boost::function<void(params&, const params&, const Context&)> Fun;

struct FunTuple {
	typedef std::vector<FunTuple> FunList;
	FunList d_children;
	Fun d_fun;
    // FunTuple(Fun fun) : d_fun(fun) { }
    void apply(params& ret, const Context&);
    void compile(const boost::property_tree::ptree&);
    void compile(const char*);
    void compile(const boost::property_tree::ptree::value_type&);
};


}
