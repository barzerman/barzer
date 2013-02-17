
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include "ay_headers.h"
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

/// skippedvector is an abstraction on top of const vector& 
/// it's needed whenever we're producing subranges of elements of a constant vecctor and passing them
/// around . Otherwise we'd have to copy 
template <typename T>
struct skippedvector {
    typedef const std::vector<T> Vector;
    const Vector* d_vec;
    size_t d_skip;


    skippedvector( const Vector* v, size_t skip=0 ) : 
        d_vec(v), d_skip(skip)
    {}
    skippedvector( const Vector& v, size_t skip=0 ) : 
        d_vec(&v), d_skip(skip)
    {}

    skippedvector& incrementSkip( size_t i ) { d_skip += i; }
    typename Vector::const_iterator begin()  const { return (d_vec->begin()+d_skip); }
    typename Vector::const_iterator end()    const { return (d_vec->end()); }

    const T& operator[]( size_t i ) const { return (*d_vec)[ i+d_skip ]; }

    const Vector* vecPtr() const { return d_vec; }
    const Vector& vec() const { return *d_vec; }
    const size_t  size() const { return ( d_vec->size()> d_skip ? d_vec->size()-d_skip: 0 ); }
    size_t getSkip() const { return d_skip; }
};

} // namespace ay
