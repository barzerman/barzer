/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once 
#include <barzer_el_pattern.h>
namespace barzer {

// Punctuation and Stop Tokens (theChar is 0 for stops) 
struct BTND_Pattern_Punct : public BTND_Pattern_Base {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	int theChar; // actual punctuation character. 0 - same as stop
	
	BTND_Pattern_Punct() : theChar(0xffffffff) {}
	BTND_Pattern_Punct(char c) : theChar(c) {}

	void setChar( char c ) { theChar = c; }
    int getChar() const { return theChar; }
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Punct& x )
	{ return( fp << "'" << std::hex << x.theChar << "'" ); }

/// this class BTND_Pattern_CompoundedWord is currently UNUSED
struct BTND_Pattern_CompoundedWord : public BTND_Pattern_Base {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	uint32_t compWordId;  
	BTND_Pattern_CompoundedWord() :
		compWordId(0xffffffff)
	{}
	BTND_Pattern_CompoundedWord(uint32_t cwi ) : compWordId(cwi) {}
};
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_CompoundedWord& x )
	{ return( fp << "compw[" << std::hex << x.compWordId << "]");}

struct BTND_Pattern_Meaning : public BTND_Pattern_Base {
    uint32_t meaningId; /// 0xffffffff - default value means any meaning is acceptable 

    BTND_Pattern_Meaning(): meaningId(0xffffffff) {}
    std::ostream& print( std::ostream& fp, const BELPrintContext& ctxt ) const
    {
        return ( fp << "m:" << meaningId << std::endl );
    }
};
struct BTND_Pattern_Token : public BTND_Pattern_Base {
	std::ostream& print( std::ostream&, const BELPrintContext& ) const;
	ay::UniqueCharPool::StrId stringId;
	bool doStem;

	BTND_Pattern_Token() : 
		stringId(0xffffffff), doStem(false)
	{}
	BTND_Pattern_Token(ay::UniqueCharPool::StrId id) : 
		stringId(id),
		doStem(false)
	{}
    ay::UniqueCharPool::StrId getStringId() const { return stringId; }
};
/// stop token is a regular token literal except it will be ignored 
/// by any tag matching algorithm
struct BTND_Pattern_StopToken : public BTND_Pattern_Token {
	std::ostream& print( std::ostream& fp, const BELPrintContext& ctxt ) const
	{
		return BTND_Pattern_Token::print( fp, ctxt ) << "<STOP>";
	}
	//ay::UniqueCharPool::StrId stringId;

	BTND_Pattern_StopToken() : BTND_Pattern_Token(0xffffffff) {}
	BTND_Pattern_StopToken(ay::UniqueCharPool::StrId id) : 
		BTND_Pattern_Token(id)
	{}
};

inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_StopToken& x )
	{ return( fp << "stop[" << std::hex << x.stringId << "]");}
inline std::ostream& operator <<( std::ostream& fp, const BTND_Pattern_Token& x )
	{ return( fp << "string[" << std::hex << x.stringId << "]");}

} // namespace barzer
