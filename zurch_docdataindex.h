
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 

#pragma once

#include <functional>
#include <boost/variant.hpp>
#include <boost/unordered_map.hpp>

namespace zurch
{
typedef boost::variant<int, double, std::string> DataType_t;

typedef boost::unordered_map<std::string, DataType_t> DocInfo;

namespace Filters
{
	template<template<typename Data> class Op>
	struct BinaryApplier : boost::static_visitor<bool>
	{
		template<typename T>
		bool operator()(const T& t1, const T& t2) const { return Op<T>()(t1, t2); }
		
		template<typename T, typename U>
		bool operator()(const T&, const U&) const { return false; }
	};
	
	template<template<typename Data> class Op>
	struct Ord
	{
		const std::string m_prop;
		const DataType_t m_val;
		
		Ord(const std::string& prop, const DataType_t& val)
		: m_prop(prop)
		, m_val(val)
		{
		}
		
		bool operator()(const DocInfo& di) const
		{
			auto pos = di.find(m_prop);
			if (pos == di.end())
				return false;
			
			return boost::apply_visitor(BinaryApplier<Op>(), pos->second, m_val);
		}
	};

	struct FilterTApplier : boost::static_visitor<bool>
	{
		const DocInfo& m_di;
		
		FilterTApplier(const DocInfo& di) : m_di(di) { }
		
		template<typename T>
		bool operator()(const T& t) const { return t(m_di); }
	};

	template<template<typename Data> class Op>
	struct LogicalBinary;
	
	struct Not;
	struct Between;

	typedef boost::variant<
			Ord<std::equal_to>,
			Ord<std::greater>,
			Ord<std::less>,
			boost::recursive_wrapper<LogicalBinary<std::logical_or>>,
			boost::recursive_wrapper<LogicalBinary<std::logical_and>>,
			boost::recursive_wrapper<Not>,
			boost::recursive_wrapper<Between>
		> Filter_t;

	template<template<typename Data> class Op>
	struct LogicalBinary
	{
		const Filter_t m_f1;
		const Filter_t m_f2;
		
		LogicalBinary(const Filter_t& f1, const Filter_t& f2) : m_f1(f1), m_f2(f2) { }
		
		bool operator()(const DocInfo& di) const
		{
			FilterTApplier app(di);
			return Op<bool>()(boost::apply_visitor(app, m_f1), boost::apply_visitor(app, m_f2));
		}
	};
	
	struct Not
	{
		const Filter_t m_f;
		
		Not(const Filter_t& f) : m_f (f) {}
		
		bool operator()(const DocInfo& di) const
		{
			return boost::apply_visitor(FilterTApplier(di), m_f);
		}
	};
	
	struct Between : LogicalBinary<std::logical_and>
	{
		Between(const std::string& prop, const DataType_t& lower, const DataType_t& upper)
		: LogicalBinary(Ord<std::greater>(prop, lower), Ord<std::less>(prop, upper))
		{
		}
	};
	
	typedef Ord<std::equal_to> EQ;
	typedef Ord<std::greater> Greater;
	typedef Ord<std::less> Less;
	typedef LogicalBinary<std::logical_or> OR;
	typedef LogicalBinary<std::logical_and> AND;
}

class DocDataIndex
{
	boost::unordered_map<uint32_t, DocInfo> m_index;
public:
	void addInfo(uint32_t, const DocInfo::value_type&);
	
	bool operator()(uint32_t, const Filters::Filter_t&) const;
};

}
