#ifndef AY_RAII_H
#define AY_RAII_H

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

}
#endif // AY_RAII_H
