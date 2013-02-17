
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
/// a few RAII primitives 

namespace ay {

template <typename T>
struct valkeep {
	const T oldVal;
	T& underlying;

	explicit valkeep( T& v, const T& newVal ) : 
		oldVal(v),
		underlying(v=newVal)
	{}
	
	~valkeep() { underlying= oldVal; }
};

} // namespace ay
