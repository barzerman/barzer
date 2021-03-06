
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 

#pragma once

#include <functional>
#include <boost/variant.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/insert_range.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/transform.hpp>
#include <ay/ay_index.h>
#include <ay/ay_tagindex.h>

namespace zurch
{
typedef boost::variant<int, double, std::string> DataType_t;

typedef boost::unordered_map<std::string, DataType_t> DocInfo;

#ifdef ENABLE_ZURCH_FILTERS
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

	template<template<typename Data> class Op> struct LogicalBinary;
	template<typename T> struct InSet;
	struct Not;
	struct Between;

	typedef boost::mpl::vector<
			Ord<std::equal_to>,
			Ord<std::greater>,
			Ord<std::less>,
			boost::recursive_wrapper<LogicalBinary<std::logical_or>>,
			boost::recursive_wrapper<LogicalBinary<std::logical_and>>,
			boost::recursive_wrapper<Not>,
			boost::recursive_wrapper<Between>
		> _PreFilterTypesVec_1;
	
	template<template<typename U> class Templ>
	struct InstTemplate
	{
		template<typename T>
		struct Inst
		{
			typedef boost::recursive_wrapper<Templ<T>> type;
		};
	};
	
	typedef boost::mpl::insert_range<_PreFilterTypesVec_1,
			boost::mpl::end<_PreFilterTypesVec_1>::type,
			boost::mpl::transform<DataType_t::types, InstTemplate<InSet>::Inst<boost::mpl::_1>>::type>::type FilterTypesVec_t;
	
	typedef boost::make_variant_over<FilterTypesVec_t>::type Filter_t;

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
			return !boost::apply_visitor(FilterTApplier(di), m_f);
		}
	};
	
	struct Between : LogicalBinary<std::logical_and>
	{
		Between(const std::string& prop, const DataType_t& lower, const DataType_t& upper)
		: LogicalBinary(Ord<std::greater>(prop, lower), Ord<std::less>(prop, upper))
		{
		}
	};
	
	template<typename T>
	struct SetFinder : public boost::static_visitor<bool>
	{
		const boost::unordered_set<T>& m_set;
		
		SetFinder(const boost::unordered_set<T>& set) : m_set(set) {}
		
		bool operator()(const T& t) const { return m_set.find(t) != m_set.end(); }
		
		template<typename U>
		bool operator()(const U&) const { return false; }
	};
	
	template<typename T>
	struct InSet
	{
		const std::string m_prop;
		const boost::unordered_set<T> m_set;
		
		InSet(const std::string& prop, const boost::unordered_set<T>& set)
		: m_prop(prop)
		, m_set(set)
		{
		}
		
		bool operator()(const DocInfo& di) const
		{
			auto pos = di.find(m_prop);
			if (pos == di.end())
				return false;
			
			return boost::apply_visitor(SetFinder<T>(m_set), pos->second);
		}
	};
	
	typedef Ord<std::equal_to> EQ;
	typedef Ord<std::greater> Greater;
	typedef Ord<std::less> Less;
	typedef LogicalBinary<std::logical_or> OR;
	typedef LogicalBinary<std::logical_and> AND;
}
#endif /// ENABLE_ZURCH_FILTERS

struct SimpleIdx {
    ay::IdPropValIndex<int>         Int;
    ay::IdPropValIndex<double>      Double;
    ay::IdPropValIndex<std::string> String;

    typedef ay::IdPropValIndex<int>::IdxType_t int_t;
    typedef ay::IdPropValIndex<double>::IdxType_t double_t;
    typedef ay::IdPropValIndex<std::string>::IdxType_t string_t;

    const int_t*    ix_int( const std::string& propName ) const { return Int.getPropIdx( propName ); }
    const double_t* ix_double( const std::string& propName ) const { return Double.getPropIdx( propName ); }
    const string_t* ix_string( const std::string& propName ) const { return String.getPropIdx( propName ); }

    int_t*    ix_int( const std::string& propName ) { return Int.getPropIdx( propName ); }
    double_t* ix_double( const std::string& propName ) { return Double.getPropIdx( propName ); }
    string_t* ix_string( const std::string& propName ) { return String.getPropIdx( propName ); }
    void sort() { 
        Int.sort();
        Double.sort();
        String.sort();
    }
};

class DocDataIndex
{
	boost::unordered_map<uint32_t, DocInfo> m_index;
    SimpleIdx d_simpleIdx;

    ay::tagindex<uint32_t>  d_tagIdx;
public:
          SimpleIdx& simpleIdx()       { return d_simpleIdx; }
    const SimpleIdx& simpleIdx() const { return d_simpleIdx; }

	void addInfo(uint32_t, const DocInfo::value_type&);
	
    /// lets not call t () operator 
#ifdef ENABLE_ZURCH_FILTERS
	bool fancyFilter(uint32_t, const Filters::Filter_t&) const;
#endif
};

}
