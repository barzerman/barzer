
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include "ay_vector.h"
#include <stack>
#include <iterator>

namespace ay {

/// trie_node has a vecmap of children
template <typename K, typename D, typename Comp=std::less<K> >
class trie
{
	D d_data;
public:
	typedef vecmap< K, boost::recursive_wrapper<trie>, Comp > VecMap;
protected:
	VecMap d_child;
public:
	size_t getNumChildren() const { return d_child.size(); }
	bool isLeaf() const { return !d_child.size(); }

	typedef typename VecMap::value_type value_type;

	typename VecMap::value_type& operator[]( size_t i ) { return d_child[i]; }
	const typename VecMap::value_type& operator[]( size_t i ) const { return d_child[i]; }

	typename VecMap::const_iterator childBegin() const { return d_child.begin(); }
	typename VecMap::iterator childBegin() { return d_child.begin(); }

	typename VecMap::const_iterator childEnd() const { return d_child.end(); }
	typename VecMap::iterator childEnd() { return d_child.end(); }

	typedef K key_type;
	typedef D data_type;
	
	typedef std::vector< typename VecMap::iterator > Path;

	trie( const data_type& d ) : d_data(d) {}
	trie( ) {}

	data_type& data() { return d_data; }
	const data_type& data() const { return d_data; }

	trie& add( const key_type& key, const data_type& d )
	{ return (d_child.insert( key, boost::recursive_wrapper<trie>(d) ).first->second).get(); }

	template<typename Updater>
	trie& addWithUpdate(const key_type& key, Updater upd)
	{
		typename VecMap::iterator pos = d_child.find(key);
		if (pos == d_child.end())
			return add(key, upd(data_type()));
		else
		{
			pos->second.data() = upd(pos->second.data());
			return pos->second;
		}
	}

	typename VecMap::const_iterator find( const  key_type& key ) const 
		{ return d_child.find( key ); }
	typename VecMap::iterator find( const  key_type& key ) 
		{ return d_child.find( key ); }
	
	// returns the pair of bool - whether or not anything at all was found 
	// and the new end 
	template <class FI>
	std::pair< const trie*, FI> getLongestPath( FI start, FI end, const trie* found=0 ) const
	{
		if( start == end ) 
			return std::pair< const trie*, FI>( found, end );

		typename VecMap::const_iterator i = find( *start );
		if( i == d_child.end() ) 
			return std::pair< const trie*, FI>( found, start );
		else
			return i->second.getLongestPath( ++start, end, &(i->second) );
	}

	/// EV is the evaluator bool EV::operator()( i->second ) 
	/// when this is true found will be advanced to i->second
	template <class FI>
	std::pair< const trie*, FI> getTerminatedLongestPath( FI start, typename std::iterator_traits<FI>::value_type end, const trie* found=0 ) const
	{
		if( *start == end ) 
			return std::pair< const trie*, FI>( found, start );

		typename VecMap::const_iterator i = find( *start );
		if( i == d_child.end() ) 
			return std::pair< const trie*, FI>( found, start );
		else
			return i->second.get().getTerminatedLongestPath( ++start, end, &(i->second.get()) );
	}

	/// FI must be an iterator such that *FI is pair<key_type,data_type>
	template <class FI>
	trie& addPath( FI start, FI end )
	{
		if( start == end ) 
			return *this;
		else {
			trie& v = add( start->first, start->second );
			return v.addPath( ++start, end );
		}
	}
	/// FI must be an iterator such that *FI is key_type
	template <class FI>
	trie& addKeyPath( FI start, FI end, const data_type& dfltV = data_type() )
	{
		if( start == end ) 
			return *this;
		else {
			trie& v = add( *start, dfltV );
			return v.addKeyPath( ++start, end );
		}
	}
	/// 
	template <class FI>
	trie& addTerminatedKeyPath( FI start, typename std::iterator_traits<FI>::value_type stopV, const data_type& dfltV = data_type() )
	{
		if( *start == stopV ) 
			return *this;
		else {
			trie& v = add( *start, dfltV );
			return v.addTerminatedKeyPath( ++start, stopV, dfltV );
		}
	}
	
};

typedef enum {
	TRIE_VISITOR_CONTINUE,
	TRIE_VISITOR_PRUNE,
	TRIE_VISITOR_ABORT
} trie_visitor_continue_t;

/// T must be an ay::trie<>
/// C(allback) must have trie_visitor_continue_t ()(std::vector< T::VecMap::iterator > ) 
/// which will be invoked for every node of the trie
/// first time false is returned from callback traversal is aborted 
template <typename T,typename C>
struct trie_visitor {

	typedef T trie_type;
	typedef C callback_type;

	typedef typename T::VecMap::iterator trie_child_iterator;

	typedef std::vector< trie_child_iterator > Path;
	Path d_stack;

	callback_type d_cb;

	trie_visitor( callback_type& cb ) : d_cb(cb) {}

	bool visit( trie_type& t )
	{
		for( trie_child_iterator i = t.childBegin(); i != t.childEnd(); ++i ) {
			d_stack.push_back( i );
			int cbRc =d_cb( d_stack ) ;
			switch( cbRc ) {
			case TRIE_VISITOR_CONTINUE:
				visit( i->second );
				d_stack.pop_back( );
				break;
			case TRIE_VISITOR_PRUNE: d_stack.pop_back( ); break;
			case TRIE_VISITOR_ABORT: d_stack.pop_back( ); return false;
			}
		}
		return true;
	}
	bool visit_leaves( trie_type& t )
	{
		for( trie_child_iterator i = t.childBegin(); i != t.childEnd(); ++i ) {
			if( i->second.isLeaf() ) {
				d_stack.push_back( i );
			
				int cbRc =d_cb( d_stack ) ;
				d_stack.pop_back( ); 
				if( cbRc == TRIE_VISITOR_ABORT ) return false;
			} else
				visit_leaves( i->second );
		}
		return true;
	}
};
/// char trie 

template <typename D>
struct char_trie_funcs {
	typedef trie<char, D> Trie;
	static void add( Trie& trie, const char* s, const D& d, const D& dfltV= D() )
		{ trie.addTerminatedKeyPath( s, ((char)0), dfltV ).data() = d; }
	
	static std::pair< const Trie*, const char*> matchString( const Trie& trie, const char* s ) 
	{
		return trie.getTerminatedLongestPath( s, (char)(0));
	}
};

template<typename D>
class char_trie : public trie<char, D>
{
public:
	void add(const char *s, const D& d, const D& defValue = D())
	{
		char_trie_funcs<D>::add(*this, s, d, defValue);
	}
	
	std::pair<const trie<char, D>*, const char*> matchString(const char *s) const
	{
		return char_trie_funcs<D>::matchString(*this, s);
	}
};

} // namespace ay
