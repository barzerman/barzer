#ifndef AY_TRIE_H
#define AY_TRIE_H

#include <ay_vector.h>
#include <stack>

namespace ay {

/// trie_node has a vecmap of children
template <typename K, typename D, typename Comp=std::less<K> >
class trie
{
	D d_data;
public:
	typedef vecmap< K, trie, Comp > VecMap;
private:
	VecMap d_child;
public:
	size_t getNumChildren() const { return d_child.size(); }
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
	{ return (d_child.insert( key, d ).first->second); }

	typename VecMap::const_iterator find( const  key_type& key ) const 
		{ return d_child.find( key ); }
	typename VecMap::iterator find( const  key_type& key ) 
		{ return d_child.find( key ); }
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
};

} // namespace ay
#endif // AY_TRIE_H
