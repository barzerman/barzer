#ifndef AY_CHOOSE_H
#define AY_CHOOSE_H

#include <iostream>
#include <vector>
#include <stack>

namespace ay {

///////// 
/////////                        choose_n
///////////////////////////////////////////////////

/// the choose_n class is a functor which, given a range of iterators over type T 
/// produces all subsequences of elements pointed to by the iterators of this range 
/// of lengths within a given range of lengths 
/// for every matching subsequence (choice) the callback functor will be invoked
/// 
/// In other words, given a collection of elements it produces sub-collections of sizes 
/// between n and m 
/// 
/// produces all choose input sequence between d_minN and d_maxN
/// 
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
				++d_numChoices;
			}
		}
		if( fromI!= toI && d_result.size() < d_maxN ) {
			/// needed = min - d_result.size
			/// fromI, toI - needed
			size_t needed = ( d_minN > d_result.size() ? d_minN - d_result.size():0 );	
			size_t haveMore = toI - fromI;

			if( needed <= haveMore ) {
				for( Iter i = fromI; i!= toI; ++i ) {
					d_result.push_back( *i );
					recurse( (i+1), toI );
					d_result.pop_back();
				}
			}
		}
	}
	template <typename Iter>
	void doAllWithoutOne( Iter fromI, Iter toI) 
	{
		for( Iter i = fromI; i!= toI; ++i ) {
			d_result.clear();
			for( Iter j = fromI; j!= toI; ++j ) {
				if( j!= i ) 
					d_result.push_back( *j );
			}
			d_callback( d_result.begin(), d_result.end() );
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
		if( d_minN == d_maxN && d_minN+1 == (size_t)(toI-fromI) ) {
			doAllWithoutOne( fromI, toI );
		} else {
			recurse( fromI, toI );
		}
	}
	size_t getNumChoices() const { return d_numChoices; }
};

typedef choose_n<char, iterator_range_print_callback<char> > choose_n_char_print;

template <typename T, typename CB>
struct unique_chars {
	std::vector<T> d_result;
	CB& d_callback;
    size_t d_numDups;

	unique_chars( CB& cb ) :
		d_callback(cb),
        d_numDups(0)
	{}	
    void clear() { d_result.clear(); d_numDups=0; }
	template <typename Iter>
	void operator()( Iter fromI, Iter toI ) {
		clear();
        for( Iter i=fromI; i!= toI; ++i ) {
            if( d_result.size() ) {
                if( (*d_result.rbegin()) == *i ) {
                    ++d_numDups;
                } else {
                    d_result.push_back(*i);
                }
            } else {
                d_result.push_back( *i );
            }
        }
        if( d_numDups )
	        d_callback( d_result.begin(), d_result.end() );
    }
    size_t getNumDups() const { return d_numDups; }

    const std::vector<T>& result() const { return d_result; }
    typedef T char_type;
    size_t                result_sz() const { return d_result.size(); }
};
} // end of anon namespace 

#ifdef TEST_AY_CHOOSE_CPP
#include <stdlib.h>
int main( int argc, char *argv[] )
{
	ay::iterator_range_print_callback<char> printCb( std::cerr );

	std::string buf;
	while( std::cin >> buf ) {
		if( buf.length() > 3 ) {	
			size_t minSubstrLen = buf.length() -1;

			ay::choose_n_char_print chooser( printCb, minSubstrLen-1, minSubstrLen );
			chooser( buf.begin(), buf.end() ) ;
			std::cerr << chooser.getNumChoices() << " choices produced\n";
		}
	}
	return 0;
}
#endif // TEST_AY_CHOOSE_CPP

#endif // AY_CHOOSE_H
