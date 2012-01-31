#ifndef AY_SCHROD_H
#define AY_SCHROD_H

// this implements a Schroedinger Object - a wrapper over a type 
// which takes construction argument alongside the boolean telling it whether or not the actual object
// should be constructed on stack
// for example you have a boost mutex lock or some other RAII wrapper, which you want to conditionally 
// do nothing - meaning - you dont want it to actually lock anything 
// schrod<Lock> lock( mutex, false ) would do the job
// the constructor wont actually create Lock(mutex) nor will the destructor destroy it 
namespace ay {

template <typename Actual>
class Schrod {
	char d_buf[ sizeof(Actual) ];
	bool d_exists;

	Schrod& operator=( const Schrod& ) { return *this; }
	Schrod(const Schrod& ) {}
public:
	typedef Actual value_type;
	Schrod( bool ex=true  ) : d_exists(ex) { if( d_exists) new(d_buf) Actual(); }

	template <typename T> Schrod( T& p, bool ex=true  ) : d_exists(ex) { if( d_exists) new(d_buf) Actual(p); }
	template <typename T> Schrod( const T& p, bool ex=true  ) : d_exists(ex) { if( d_exists) new(d_buf) Actual(p); }

	bool exists() const { return d_exists; }
	Actual* actual() { return ( d_exists? (Actual*)d_buf : 0 ); }
	const Actual* actual() const { return ( d_exists? (Actual*)d_buf : 0 ); }

	~Schrod() { if( d_exists ) (static_cast<Actual*>(d_buf))->Actual::~Actual(); }
};

}
#endif // AY_SCHROD_H
