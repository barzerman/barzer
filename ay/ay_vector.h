#ifndef AY_VECTOR_H
#define AY_VECTOR_H

#include <ay_headers.h>
#include <vector>
#include <boost/variant.hpp>


// this file has various vector-based containers such as 
// slogrovector, vecmap etc 

// slogrovector is something that starts with a presumed size and then 
// increments capacity by a fixed number of elements instead of doubling
// this is needed for mostly static large vectors , whose approximate future size 
// is known. it's just a simple wrap of a traditional vector 
namespace ay {

template <typename T>
struct slogrovector {
public:
	std::vector<T> vec;
	size_t capacityIncrement;

private:
	inline void tryReserve() {
		if( vec.size() >= vec.capacity() )  {
			if( capacityIncrement )
				vec.reserve( vec.size()  + capacityIncrement );
		}
	}
public:
	slogrovector( size_t initCap, size_t capInc ) : 
		capacityIncrement( capInc) 
	{ vec.reserve(initCap); }

	slogrovector( )  : capacityIncrement(0) {}
	slogrovector( size_t capInc ) : 
		capacityIncrement(capInc) 
	{ }
	void setCapacity( size_t initCap, size_t capInc ) 
	{
		if( initCap > vec.capacity() ) 
			vec.reserve(initCap);
		capacityIncrement = capInc;
	}
	
	void push_back( const T& t )  {
		tryReserve();
		vec.push_back( t );
	}
	/// adds one default element at the end 
	inline T& extend() {
		tryReserve();
		vec.resize( vec.size() +1 ) ;
		return vec.back();
	}
};

/// vecmap keeps the vector of Key Value pairs with unique keys
/// it mainly implements the basic iterations
template <typename K, typename V, typename Comp = std::less<K> >
class vecmap {
public:
	typedef std::pair< K,V> value_type;
	typedef V data_type;
	typedef K key_type;
	typedef Comp key_comp;

	struct value_comp {
		bool operator() ( const value_type& l, const value_type& r ) const
		{ return (key_comp()( l.first, r.first )); }

		bool operator() ( const value_type& l, const key_type& r ) const
		{ return (key_comp()( l.first, r )); }
		bool operator() ( const key_type& l, const value_type& r ) const
		{ return (key_comp()( l, r.first )); }
		
	};

	typedef std::vector< value_type > TheVec;
	
	typedef typename TheVec::iterator iterator;
	typedef typename TheVec::const_iterator const_iterator;
private:
	TheVec d_vec;
public:
	iterator begin() { return d_vec.begin(); }
	const_iterator begin() const { return d_vec.begin(); }

	iterator end() { return d_vec.end(); }
	const_iterator end() const { return d_vec.end(); }
	
	iterator lower_bound( const K& k ) 
		{ return std::lower_bound( d_vec.begin(), d_vec.end(), k, value_comp() ); }
	const_iterator lower_bound( const K& k ) const
		{ return std::lower_bound( d_vec.begin(), d_vec.end(), k, value_comp() ); }

	iterator find( const K& k ) 
	{ iterator i = lower_bound(k); return( (i == d_vec.end() || value_comp()(k,*i) ) ?  d_vec.end():i) ; }
	const_iterator find( const K& k ) const
	{ const_iterator i = lower_bound(k); return( (i == d_vec.end() || value_comp()(k,*i) ) ?  d_vec.end():i) ; }
	
	std::pair< iterator, iterator > equal_range( const key_type& k ) 
		{ return std::equal_range( d_vec.begin(), d_vec.end(), k, value_comp() ); }
	std::pair< const_iterator, const_iterator > equal_range( const key_type& k ) const
		{ return std::equal_range( d_vec.begin(), d_vec.end(), k, value_comp() ); }

	iterator upper_bound( const K& k ) 
		{ return std::upper_bound( d_vec.begin(), d_vec.end(), k, value_comp() ); }
	const_iterator upper_bound( const K& k ) const
		{ return std::upper_bound( d_vec.begin(), d_vec.end(), k, value_comp() ); }

	std::pair< iterator, bool> insert( const value_type& v )
	{
		iterator i = lower_bound( v.first );
		if( i == d_vec.end() ||  value_comp()( v.first,*i) ) { // first non smaller is not equal or all keys smaller
			return std::pair< iterator, bool>( d_vec.insert( i, v ), true );
		} else {
			return std::pair< iterator, bool>( i, false );
		}
	}
	std::pair< iterator, bool> insert( const key_type& k, const data_type& d  )
	{
		iterator i = lower_bound( k );
		if( i == d_vec.end() ||  value_comp()(k,*i) ) { // first non smaller is not equal or all keys smaller
			return std::pair< iterator, bool>( d_vec.insert( i, value_type(k,d) ), true );
		} else {
			return std::pair< iterator, bool>( i, false );
		}
	}


	void clear() { d_vec.clear(); }
	iterator erase( iterator pos ) { return d_vec.erase(pos); }
	iterator erase( iterator first, iterator last ) { return d_vec.erase(first,last); }

	size_t size() const { return d_vec.size(); }

	/// i has to be a number, not K
	value_type& operator[] ( size_t i ) { return d_vec[i]; }
	const value_type& operator[] ( size_t i ) const { return d_vec[i]; }
};
/// sorted vector
template <typename T, typename Comp = std::less<T> >
class vecset {
	typedef T value_type;
	typedef std::vector<value_type> TheVec;	
	typedef Comp value_comp;
	TheVec d_vec;
public:
	const TheVec& getVector() const { return d_vec; }
	void clear() { d_vec.clear(); }
	typedef typename TheVec::iterator iterator;
	typedef typename TheVec::const_iterator const_iterator;

	iterator begin() { return d_vec.begin(); }
	const_iterator begin() const { return d_vec.begin(); }

	iterator end() { return d_vec.end(); }
	const_iterator end() const { return d_vec.end(); }
	
	iterator lower_bound( const T& k ) 
		{ return std::lower_bound( d_vec.begin(), d_vec.end(), k, value_comp() ); }
	const_iterator lower_bound( const T& k ) const
		{ return std::lower_bound( d_vec.begin(), d_vec.end(), k, value_comp() ); }

	iterator find( const T& k ) 
	{ iterator i = lower_bound(k); return( (i == d_vec.end() || value_comp()(k,*i) ) ?  d_vec.end():i) ; }
	const_iterator find( const T& k ) const
	{ const_iterator i = lower_bound(k); return( (i == d_vec.end() || value_comp()(k,*i) ) ?  d_vec.end():i) ; }
	
	std::pair< iterator, iterator > equal_range( const value_type& k ) 
		{ return std::equal_range( d_vec.begin(), d_vec.end(), k, value_comp() ); }
	std::pair< const_iterator, const_iterator > equal_range( const value_type& k ) const
		{ return std::equal_range( d_vec.begin(), d_vec.end(), k, value_comp() ); }

	iterator upper_bound( const T& k ) 
		{ return std::upper_bound( d_vec.begin(), d_vec.end(), k, value_comp() ); }
	const_iterator upper_bound( const T& k ) const
		{ return std::upper_bound( d_vec.begin(), d_vec.end(), k, value_comp() ); }

	std::pair< iterator, bool> insert( const value_type& v )
	{
		iterator i = lower_bound( v );
		if( i == d_vec.end() ||  value_comp()( v,*i) ) { // first non smaller is not equal or all keys smaller
			return std::pair< iterator, bool>( d_vec.insert( i, v ), true );
		} else {
			return std::pair< iterator, bool>( i, false );
		}
	}
	template <typename InputIterator> 
	void insert( InputIterator first, InputIterator last )
	{
		for( InputIterator i = first; i!= last; ++i ) 
			insert( *i );
	}

	iterator erase( iterator pos ) { return d_vec.erase(pos); }
	iterator erase( iterator first, iterator last ) { return d_vec.erase(first,last); }

	size_t size() const { return d_vec.size(); }

	/// i has to be a number, not T
	value_type& operator[] ( size_t i ) { return d_vec[i]; }
	const value_type& operator[] ( size_t i ) const { return d_vec[i]; }
	
};

// evovec - evolving vector
/// sizeof evovec is the same as sizeof(vector) but if the contained type is small enough
/// evovec may keep contained objects in place (no memory allocation) 
/// 
/// but for small types it can hold a few elements
template <typename T>
class evovec {
public:
	typedef std::vector<T> vec_type;
	
	struct fixvec {
		enum { INTARR_CAPACITY = (sizeof(vec_type)-1)/sizeof(T) };
		T dta[ INTARR_CAPACITY ];
		uint8_t  sz;

		uint8_t size() const { return sz; } 
		uint8_t capacity() const { return INTARR_CAPACITY; } 
		bool hasCapacity() const { return (sz< INTARR_CAPACITY); }

		bool append( const T& n ) 
			{ return ( hasCapacity() ? (dta[sz++]= n, true) : false ); }

		const T* begin_ptr() const { return &(dta[0]); }
		const T* end_ptr() const { return &(dta[sz]); }

		T* begin_ptr() { return &(dta[0]); }
		T* end_ptr() { return &(dta[sz]); }

		fixvec() : sz(0) {}
	};
	typedef boost::variant< fixvec, vec_type>  variant_type;

	typedef std::pair< T*, T* > range;
	typedef std::pair< const T*, const T* > range_const;

	variant_type data;	
/// public interface
	bool is_fixvec() const { return !data.which(); }
	bool is_vector() const { return data.which(); }

	vec_type* get_vector_ptr( ) { return boost::get< vec_type >( &data ); }
	const vec_type* get_vector_ptr() const{ return boost::get< vec_type >( &data ); }

	vec_type& get_vector( ) { return boost::get< vec_type >( data ); }
	const vec_type& get_vector() const{ return boost::get< vec_type >( data ); }

	fixvec* get_fixvec_ptr() { return boost::get< fixvec >( &data ); }
	const fixvec* get_fixvec_ptr() const { return boost::get< fixvec >( &data ); }

	fixvec& get_fixvec() { return boost::get< fixvec >( data ); }
	const fixvec& get_fixvec() const { return boost::get< fixvec >( data ); }

	void push_back( const T& n ) { 
		if( is_fixvec() ) { /// still in fixvec 
			fixvec& fv = boost::get< fixvec >(data);
			if( !fv.append(n) ) { // array becomes vector when this returns false
				vec_type tmpVec;
				tmpVec.reserve( fv.size() +1 ); // minimizes allocations
				tmpVec.insert( tmpVec.end(), fv.begin_ptr(), fv.end_ptr() );

				data = vec_type();
				vec_type& vec = get_vector();
				vec.swap( tmpVec );
				
				vec.push_back(n);
			}
		} else { // already a vector
			get_vector().push_back(n);
		}
	}

	const T* begin_ptr() const 
		{ 
			const fixvec* fv = get_fixvec_ptr();
			return ( fv ? fv->begin_ptr() : &(get_vector().front()) );
		}
	T* begin_ptr() 
		{ 
			fixvec* fv = get_fixvec_ptr();
			return ( fv ? fv->begin_ptr() : &(get_vector().front()) );
		}
	const T* end_ptr() const 
		{ 
			const fixvec* fv = get_fixvec_ptr();
			return ( fv ? fv->end_ptr() : &(get_vector().back())+1 );
		}
	T* end_ptr() 
		{ 
			fixvec* fv = get_fixvec_ptr();
			return ( fv ? fv->end_ptr() : &(get_vector().back())+1 );
		}

	range_const get_range() const 
	{
		const fixvec* fv = get_fixvec_ptr();
		if( fv ) 
			return range_const( fv->begin_ptr(), fv->end_ptr() );
		else {
			vec_type& v = get_vector();
			return range_const( &(v.front()), &( v.back())+1 );
		}
	}
	range get_range() 
	{
		fixvec* fv = get_fixvec_ptr();
		if( fv ) 
			return range( fv->begin_ptr(), fv->end_ptr() );
		else {
			vec_type& v = get_vector();
			return range( &(v.front()), &(v.back())+1 );
		}
	}
	size_t size() const 
		{ 
			const fixvec* fv = get_fixvec_ptr();
			return( fv ? fv->size() : get_vector().size() ) ;
		}
}; // end of evovec

typedef evovec< uint32_t > evovec_uint32;

}
#endif // AY_VECTOR_H
