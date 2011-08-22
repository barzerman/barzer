#ifndef AY_CHOOSE_H
#define AY_CHOOSE_H

#include <iostream>
#include <vector>
#include <stack>

namespace ay {

/// trivial callback for choose_n or anything else that passes back an iterator range
template <typename T>
struct iterator_range_print_callback {
	std::ostream& d_stream;
	iterator_range_print_callback( std::ostream& os ) : d_stream(os) {}

	template <typename Iter> 
	int operator()( Iter fromI, Iter toI ) 
	{
		for( Iter i = fromI; i!= toI; ++i ) 
			d_stream << *i ;
		d_stream << std::endl;
		return 0;
	}
};

/// this class is a functor which, given a range of iterators over type T 
/// produces all subsequences of elements pointed to by the iterators of this range 
/// of lengths within a given range of lengths 
/// for every matching subsequence (choice) the callback functor will be invoked
/// 
/// In other words, given a collection of elements it produces sub-collections of sizes 
/// between n and m 
/// 
/// produces all choose input sequence between d_minN and d_maxN
/// 
template <typename T, typename CB>
struct choose_n {
	size_t d_minN, d_maxN; // invokes callback for choice lengths between d_minN and d_maxN only
	size_t d_numChoices; // number of choices produced
	std::vector<T> d_result;
	CB& d_callback;
	

	template <typename Iter>
	void recurse( Iter fromI, Iter toI ) {
		if( d_result.size() >= d_minN ) {
			if( d_result.size() <= d_maxN ) {
				d_callback( d_result.begin(), d_result.end() );
			}
		}
		if( fromI!= toI && d_result.size() < d_maxN ) {
			for( Iter i = fromI; i!= toI; ++i ) {
				d_result.push_back( *i );

				Iter j = i;
				recurse( ++j, toI );
				d_result.pop_back();
			}
		}
	}
public:
	choose_n( CB& cb, size_t minN, size_t maxN ) : 
		d_minN(minN), d_maxN(maxN),
		d_numChoices(0) ,
		d_callback(cb)
	{}	
	
	size_t getMinChoiceLength() const { return d_minN; } 
	size_t getMaxChoiceLength() const { return d_maxN; } 

	void clear() { d_result.clear(); d_numChoices = 0; }
	template <typename Iter>
	void operator()( Iter fromI, Iter toI ) {
		clear();
		recurse( fromI, toI );
	}
};

typedef choose_n<char, iterator_range_print_callback<char> > choose_n_char_print;

} // end of anon namespace 

/****  usage sample 
int main( int argc, char *argv[] )
{
	choose_vector_print_cb<char> printCb( std::cerr );

	
	std::string buf;
	while( std::cin >> buf ) {
		if( buf.length() > 3 ) {	
			size_t minSubstrLen = buf.length() -1;

			choose_n_char_print chooser( printCb, minSubstrLen, minSubstrLen );
			chooser( buf.begin(), buf.end() ) ;
		}
	}
	return 0;
}
*/

#endif // AY_CHOOSE_H
