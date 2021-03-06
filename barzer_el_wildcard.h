
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <barzer_el_trie.h>
#include <ay/ay_pool_with_id.h>
#include <ay/ay_debug.h>
namespace barzer {
struct BarzelWCKey;

// barzer wildcard pool contiguously stores wildcards arranged by types 
// as well as the per-node wildcard lookup objects
// accessing wildcard by BarzelWCKey is an O(1) operation
// BTND_Pattern_XXXX wildcard types (except for the None type) there will be a 
// unique contiguous storage 
// of objects representing this pattern type
// the pool will try to eliminate duplication this will help reduce the trie as well 
// wildcard types are :
// BTND_Pattern_Number, BTND_Pattern_Date, BTND_Pattern_Time, BTND_Pattern_DateTime, BTND_Pattern_Wildcard


class BarzelWildcardPool {
	ay::PoolWithId< BarzelWCLookup > wcLookupPool;

	ay::InternerWithId<BTND_Pattern_Number> 	pool_Number;
	ay::InternerWithId<BTND_Pattern_Date> 		pool_Date;
	ay::InternerWithId<BTND_Pattern_Time> 		pool_Time;
	ay::InternerWithId<BTND_Pattern_DateTime> 	pool_DateTime;
	ay::InternerWithId<BTND_Pattern_Wildcard> 	pool_Wildcard;
	ay::InternerWithId<BTND_Pattern_Entity> 	pool_Entity;
	ay::InternerWithId<BTND_Pattern_ERCExpr> 	pool_ERCExpr;
	ay::InternerWithId<BTND_Pattern_ERC> 	pool_ERC;
	ay::InternerWithId<BTND_Pattern_EVR> 	pool_EVR;
	ay::InternerWithId<BTND_Pattern_Range> 	pool_Range;
public:
	const BTND_Pattern_EVR* 	get_BTND_Pattern_EVR( uint32_t id ) const 
		{ return pool_EVR.getObjById( id ); }
	const BTND_Pattern_ERC* 	get_BTND_Pattern_ERC( uint32_t id ) const 
		{ return pool_ERC.getObjById( id ); }
	const BTND_Pattern_ERCExpr* 	get_BTND_Pattern_ERCExpr( uint32_t id ) const 
		{ return pool_ERCExpr.getObjById( id ); }
	const BTND_Pattern_Range* 	get_BTND_Pattern_Range( uint32_t id ) const 
		{ return pool_Range.getObjById( id ); }
	const BTND_Pattern_Entity* 	get_BTND_Pattern_Entity( uint32_t id ) const 
		{ return pool_Entity.getObjById( id ); }
	const BTND_Pattern_Number* 	get_BTND_Pattern_Number( uint32_t id ) const 
		{ return pool_Number.getObjById( id ); }
	const BTND_Pattern_Date* 	get_BTND_Pattern_Date( uint32_t id ) const 
		{ return pool_Date.getObjById( id ); }
	const BTND_Pattern_Time* 	get_BTND_Pattern_Time( uint32_t	id ) const 
		{ return pool_Time.getObjById( id ); }
	const BTND_Pattern_DateTime* get_BTND_Pattern_DateTime(	uint32_t id ) const 
		{ return pool_DateTime.getObjById( id ); }
	const BTND_Pattern_Wildcard* get_BTND_Pattern_Wildcard( uint32_t id ) const 
		{ return pool_Wildcard.getObjById( id ); }

	template <typename T> 
	const T* get_BTND_Pattern( uint32_t id ) const;

	void clear() 
	{
		wcLookupPool.clear();

		pool_Number.clear();
		pool_Date.clear();
		pool_Time.clear();
		pool_DateTime.clear();
		pool_Wildcard.clear();
	}

	BarzelWCLookup* 			addWCLookup( uint32_t& id ) 
		{ return wcLookupPool.addObj( id ); }
	BarzelWCLookup* 			getWCLookup( uint32_t id ) 
		{ return wcLookupPool.getObjById(id); }
	const BarzelWCLookup* 		getWCLookup( uint32_t id ) const 
		{ return wcLookupPool.getObjById(id); }

	void produceWCKey( BarzelWCKey& key, const BTND_Pattern_Number& x );
	void produceWCKey( BarzelWCKey& key, const BTND_Pattern_Date& x );
	void produceWCKey( BarzelWCKey& key, const BTND_Pattern_Time& x);
	void produceWCKey( BarzelWCKey& key, const BTND_Pattern_DateTime& x);
	void produceWCKey( BarzelWCKey& key, const BTND_Pattern_Wildcard& x );
	void produceWCKey( BarzelWCKey& key, const BTND_Pattern_Range& x );
	void produceWCKey( BarzelWCKey& key, const BTND_Pattern_Entity& x );
	void produceWCKey( BarzelWCKey& key, const BTND_Pattern_ERCExpr& x );
	void produceWCKey( BarzelWCKey& key, const BTND_Pattern_ERC& x );
	void produceWCKey( BarzelWCKey& key, const BTND_Pattern_EVR& x );


	inline void produceWCKey( BarzelWCKey& k, const BTND_PatternData& x )
	{
		switch( x.which()) {
		case BTND_Pattern_Number_TYPE: produceWCKey(k, boost::get<BTND_Pattern_Number>(x) ); return;
		case BTND_Pattern_Wildcard_TYPE: produceWCKey(k, boost::get<BTND_Pattern_Wildcard>(x) ); return;
		case BTND_Pattern_Date_TYPE: produceWCKey(k, boost::get<BTND_Pattern_Date>(x) ); return;
		case BTND_Pattern_DateTime_TYPE: produceWCKey(k, boost::get<BTND_Pattern_DateTime>(x) ); return;
		case BTND_Pattern_Time_TYPE: produceWCKey(k, boost::get<BTND_Pattern_Time>(x) ); return;
		case BTND_Pattern_Entity_TYPE: produceWCKey(k, boost::get<BTND_Pattern_Entity>(x) ); return;
		case BTND_Pattern_Range_TYPE: produceWCKey(k, boost::get<BTND_Pattern_Range>(x) ); return;
		case BTND_Pattern_ERCExpr_TYPE: produceWCKey(k, boost::get<BTND_Pattern_ERCExpr>(x) ); return;
		case BTND_Pattern_ERC_TYPE: produceWCKey(k, boost::get<BTND_Pattern_ERC>(x) ); return;
		case BTND_Pattern_EVR_TYPE: produceWCKey(k, boost::get<BTND_Pattern_EVR>(x) ); return;
		default: k.clear(); return;
		}
	}
	
	// prints stats 
	std::ostream& print( std::ostream& fp ) const
	{
		return( fp << "total WC lookups: " << (wcLookupPool.getNumElements()) << "\n" <<
		"pool_Number:" << (pool_Number.getVecSize()) << "\n" <<
		"pool_Date:" << (pool_Date.getVecSize()) <<"\n" <<
		"pool_Time:" << (pool_Time.getVecSize()) <<"\n" <<
		"pool_DateTime:" << (pool_DateTime.getVecSize()) <<"\n" <<
		"pool_Entity:" << (pool_Entity.getVecSize()) <<"\n" <<
		"pool_ERCExpr:" << (pool_ERCExpr.getVecSize()) <<"\n" <<
		"pool_Wildcard:" << (pool_Wildcard.getVecSize()) )<< "\n" ;
	}
	std::ostream& print( std::ostream& fp, const BarzelWCKey& key, const BELPrintContext& ctxt ) const;
};
	template <>  
	inline const BTND_Pattern_EVR* 	BarzelWildcardPool::get_BTND_Pattern<BTND_Pattern_EVR>( uint32_t id ) const 
		{ return pool_EVR.getObjById( id ); }
	template <>  
	inline const BTND_Pattern_ERC* 	BarzelWildcardPool::get_BTND_Pattern<BTND_Pattern_ERC>( uint32_t id ) const 
		{ return pool_ERC.getObjById( id ); }
	template <>  
	inline const BTND_Pattern_ERCExpr* 	BarzelWildcardPool::get_BTND_Pattern<BTND_Pattern_ERCExpr>( uint32_t id ) const 
		{ return pool_ERCExpr.getObjById( id ); }
	template <>  
	inline const BTND_Pattern_Range* 	BarzelWildcardPool::get_BTND_Pattern<BTND_Pattern_Range>( uint32_t id ) const 
		{ return pool_Range.getObjById( id ); }
	template <>  
	inline const BTND_Pattern_Entity* 	BarzelWildcardPool::get_BTND_Pattern<BTND_Pattern_Entity>( uint32_t id ) const 
		{ return pool_Entity.getObjById( id ); }
	template <>  
	inline const BTND_Pattern_Number* 	BarzelWildcardPool::get_BTND_Pattern<BTND_Pattern_Number>( uint32_t id ) const 
		{ return pool_Number.getObjById( id ); }
	template <>  
	inline const BTND_Pattern_Date* 	BarzelWildcardPool::get_BTND_Pattern<BTND_Pattern_Date>( uint32_t id ) const 
		{ return pool_Date.getObjById( id ); }
	template <>  
	inline const BTND_Pattern_Time* 	BarzelWildcardPool::get_BTND_Pattern<BTND_Pattern_Time>( uint32_t	id ) const 
		{ return pool_Time.getObjById( id ); }
	template <>  
	inline const BTND_Pattern_DateTime* BarzelWildcardPool::get_BTND_Pattern<BTND_Pattern_DateTime>(	uint32_t id ) const 
		{ return pool_DateTime.getObjById( id ); }
	template <>  
	inline const BTND_Pattern_Wildcard* BarzelWildcardPool::get_BTND_Pattern<BTND_Pattern_Wildcard>( uint32_t id ) const 
		{ return pool_Wildcard.getObjById( id ); }
} // namespace barzer
