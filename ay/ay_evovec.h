#ifndef AY_EVOVEC_H
#define AY_EVOVEC_H

#include <vector>
#include <map>
namespace ay {

// evovec - evolving vector - W - is number of words it has to pack objects of type T in

template <typename T, size_t W=1>
class evovec {
public:
	typedef std::vector<T> vec_type;
	typedef vec_type*      vec_buf_type[W];

	enum { INTARR_CAPACITY = (sizeof(vec_buf_type))/sizeof(T) };

	union {
		vec_type*        vecP;
		T	             arr[INTARR_CAPACITY];
	} data;

	// 0xff - vector ...  
	// 0-0xff - array
	uint8_t mode;

	
	typedef std::pair< T*, T* > range;
	typedef std::pair< const T*, const T* > range_const;

/// public interface
	bool is_vector() const { return (mode == 0xff); }
	bool is_array() const { return !is_vector(); }

	vec_type* get_vector_ptr( ) { return( is_vector() ? data.vecP : 0 ); }
	const vec_type* get_vector_ptr() const{ return ( is_vector() ? data.vecP : 0 ); }

	vec_type& get_vector( ) { return *get_vector_ptr(); }
	const vec_type& get_vector() const{ return *get_vector_ptr(); }

	T* get_array() { return ( is_array() ? data.arr:0 );}
	const T* get_array() const { return ( is_array() ? data.arr:0 );}

	private:
	T* get_arr_begin() { return data.arr; } 
	const T* get_arr_begin() const { return data.arr; } 

	T* get_arr_end() { return &(data.arr[mode]); } 
	const T* get_arr_end() const { return &(data.arr[mode]); } 

	T* get_vec_begin() { return &(data.vecP->front()); }
	const T* get_vec_begin() const { return &(data.vecP->front()); }

	T* get_vec_end() { return &(*(data.vecP->end())); }
	const T* get_vec_end() const { return &(*(data.vecP->end())); }

	public:

	bool arr_append( const T& n ) 
	{
		return( is_array() && mode< INTARR_CAPACITY ? (get_array()[ mode ] = n,++mode,true) :false );
	}

	void clear() 
	{
		if( is_vector() ) 
			delete data.vecP;
		
		mode=0;
	}
	evovec() : mode(0) {}
	~evovec() { delete get_vector_ptr(); }

	void push_back( const T& n ) { 
		if( is_array() ) { /// still in fixvec 
			if( !arr_append(n) ) { // array becomes vector when this returns false
				vec_type* vec = new vec_type();
				vec->reserve( mode +1 ); // minimizes allocations
				vec->insert( vec->end(), get_arr_begin(), get_arr_end() );
				vec->push_back( n );

				mode = 0xff;
				data.vecP = vec;
			}
		} else { // already a vector
			data.vecP->push_back(n);
		}
	}

	const T* end_ptr() const { return ( is_array() ? get_arr_end() : get_vec_end() ); }
	const T* begin_ptr() const { return ( is_array() ? get_arr_begin() : get_vec_begin() ); }
	T* end_ptr() { return ( is_array() ? get_arr_end() : get_vec_end() ); }
	T* begin_ptr() { return ( is_array() ? get_arr_begin() : get_vec_begin() ); }

	range_const get_range() const 
	{
		if( is_array() ) 
			return range_const( get_arr_begin(), get_arr_end() );
		else 
			return range_const( get_vec_begin(), get_vec_end() );
	}
	range get_range() 
	{
		if( is_array() ) 
			return range( get_arr_begin(), get_arr_end() );
		else 
			return range( get_vec_begin(), get_vec_end() );
	}
	size_t capacity() const { return ( is_array() ? INTARR_CAPACITY: data.vecP->capacity()); }
	size_t size() const 
		{ 
			return( is_array() ? mode : data.vecP->size() );
		}

	T& operator[]( size_t n ) { return *( begin_ptr() + n ); }
	const T& operator[] ( size_t n ) const { return *( begin_ptr() + n ); }
}; // end of evovec

typedef evovec< uint32_t > evovec_uint32;

}
#endif // AY_EVOVEC_H
