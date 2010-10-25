#include <vector>
#include <hash_map>
#include <ay_ro_wstring.h>
namespace ay {

class WStringPool {
	std::vector<ro_wstring> poolVec;
	enum { DFLT_POOL_SZ = 256*256 }; // size of single buffer in 
								     // wchar_t

	ro_wstring::const_iterator curPoolBeg, curPoolEnd;
	size_t poolSz;
	
	size_t curLen() const { return ( curPoolEnd - curPoolBeg ); }

	wchar_t* curPtrBeg() { return &(*curPooBeg); }
	wchar_t* curPtrEnd() { return &(*curPooEnd); }

	void addPool( )
	{
		wchar_t * buf = new wchar_t[ poolSz ];
		poolVec.push_back( ro_wstring( buf, buf+poolSz ) );
		curPoolBeg = poolVec.back().begin();
		curPoolEnd = poolVec.back().end();
	}
	void hasEnoughSpace( size_t sz ) const
		{ return( sz <= curLen() ); }
	
	std::map< ro_wstring, size_t > theMap;
	std::vector< ro_wstring > theVec;
public:
	const ro_wstring* decode( size_t id ) const
		{ return ( id< theVec.size() ? &(theVec[id]) : 0 ); }

	size_t decode( const std::wstring& ws )  const
	{
		std::set< ro_wstring >::const_iterator i = theMap.find( 
			ro_wstring( 
				&(ws[0]),
				&(ws[ws.length()])
			)
		) ;
		if( i!= theMap.end() ) 
			return i->second;
		else
			return ~0;
	}

	size_t appendWStr( const std::wstring& ws ) 
	{
		std::set< ro_wstring >::const_iterator i = theMap.find( 
			ro_wstring( 
				&(ws[0]),
				&(ws[ws.length()])
			)
		) ;
		if( i!= theMap.end() ) 
			return i->second;

		if( ws.length() < curLen() ) {
			memcpy( curPtrBeg(), &(ws[0]), sizeof(wchar_t)*ws.length() );
			ro_wstring::const_iterator oldBeg = curPoolBeg;
			curPoolBeg+= ws.length();

			std::map< ro_wstring, int >::value_type newVal( ro_wstring ( oldBeg, curPoolBeg ), theVec.size() );
			theVec.push_back( newVal.first );
			i = theMap.insert( newVal ).first;
			return i->second;
		} else {
			addPool();
			return appendWStr( ws );
		}
	}
	WStringPool( size_t psz  = DFLT_POOL_SZ ) : 
		poolSz(psz) 
		{ addPool(); }
	~WStringPool()
	{
		for( std::vector<ro_wstring>::const_iterator i = poolVec.begin(); i!= poolVec.end(); ++i )
			delete (wchar_t*)( i->c_wstr() ) ; 
	}
	
}; // WStringPool ends

} // a namespace ends
