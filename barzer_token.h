#include <wchar.h>

namespace barzer {
	typedef unsigned char Byte;
	
	/// 12 byte type representing all flavors of tokens
	class ParsedToken {
		friend class TokenResolve ;

		type Byte PTType;
		PTType type;
		// linguistic bytes - part of speech info, conjugation etc 
		// this is main language dependent and ill be UNUSED for now 
		// this will work in conjunction with the 4 bytes in dta.token.tokLing
		Byte lingInfo[3];
		
		enum {
			PTT_TOKEN,
			PTT_BARZEWORD, // special token (compound or otherwise special)
			PTT_NUM, // int.int fixed floating point number (numbers as tags)
			PTT_CHAR,  // fixed 8 char string
			PTT_DOUBLE,  // double number (for calculations)
			PTT_INT64BIT // 64 bit integer
		};
		
	public:
		PTType getType() const
			{ return type; }
	private:
		/// this union is 64 bit long
		union {
			uint64_t numInt;

			struct {
				unsigned int id; /// id of a widechar string
				unsigned int ling; /// linguistic markup 
			} token;

			int numReal[2];
			double numDouble;
			
			struct {
				int poolId; // barzeword pool id
				int id; // barzeword id
			} barzeword;

			char fixChar[8];
		} dta;
	public:
		// comparators
		//// LESS
		struct comp_less {
			inline bool operator() ( const ParsedToken& l, const ParsedToken& r ) const 
			{
				if( l.type < r.type )
					return true;
				else if( r.type < l.type )
					return false;
				else
					return l.numInt < r.numInt;
			}
		};
		/// EQUAL
		struct comp_eq {
			inline bool operator() ( const ParsedToken& l, const ParsedToken& r ) const 
					{ return ( l.type == r.type && l.numInt == r.numInt ); }
			
		};
		// type checkers 
		inline bool isToken() 			const{ return (type == PTT_TOKEN); }
		inline bool isBarzeword() 		const{ return (type == PTT_BARZEWORD); }
		inline bool isFixedPointNum() 	const{ return (type == PTT_NUM); }
		inline bool isFixedChar() 		const{ return (type == PTT_CHAR); }
		inline bool isDouble() 			const{ return (type == PTT_DOUBLE); }
		inline bool is64BitInt() 		const{ return (type == PTT_INT64BIT); }
	
		void print( std::ostream& fp );
	};
	
	inline bool operator <( const ParsedToken& l, const ParsedToken& r )
		{ return ParsedToken::comp_less()( l, r ); }
	inline bool operator ==( const ParsedToken& l, const ParsedToken& r )
		{ return ParsedToken::comp_eq()( l, r ); }

	
	/// global token resolver
	class TokenResolve {
		static wchar* getWCar( const ParsedToken&  );
		static int printTok( std::ostream& , const PrasedToken& );
		static int readTok( ParsedToken&, std::istream& );

		/// returns the single instance of the resolver
		TokenResolve* instance() const;
	};
} // barzer namespce ends
