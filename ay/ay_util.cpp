#include <ay_util.h>
namespace ay {
int wstring_vec_read( WstringVec& vec, std::wistream& in )
{
	size_t sz = 0;
	while( std::getline( in,(vec.resize(vec.size()+1 ),vec.back()), L' ') )  
		++sz;

	return ( vec.resize(sz), vec.size() );
}
} // end of ay namespace 
