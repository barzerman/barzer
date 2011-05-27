#ifndef AY_VECTOR_H
#define AY_VECTOR_H

#include <ay_headers.h>
#include <vector>
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

}
#endif // AY_VECTOR_H
