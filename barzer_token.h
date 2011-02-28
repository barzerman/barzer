#include <wchar.h>
#include <ay_wstring_pool.h>

namespace barzer {
	typedef unsigned char Byte;
	
	/// 12 byte type representing all flavors of tokens
	class ParsedToken {
		friend class TokenResolve ;

		Byte type;

		typedef enum {
			PTT_NULL, // unresolved token
			PTT_TOKEN,
			PTT_BARZEWORD, // special token (compound or otherwise special)
			PTT_NUM, // int.int fixed floating point number (numbers as tags)
			PTT_CHAR,  // fixed 8 char string
			PTT_DOUBLE,  // double number (for calculations)
			PTT_INT64BIT, // 64 bit integer
			PTT_STRING // wchar* - allocated and owned by this object. this type is needed for 
					   // unmatched strings
		} PTType;
		// linguistic bytes - part of speech info, conjugation etc 
		// this is main language dependent and ill be UNUSED for now 
		// this will work in conjunction with the 4 bytes in dta.token.tokLing
		PTType getType() const
			{ return type; }
	private:
		Byte lingInfo[3];
		
		/// this union is 64 bit long
		union {
			wchar*   wcharPtr; // this is the only type which allocs memory
			uint64_t numInt;

			struct {
				uint32_t id; /// id of a widechar string
				uint32_t ling; /// linguistic markup 
			} token;

			int numReal[2];
			double numDouble;
			
			char fixChar[8];
		} dta;
	public:
		void clear() 
		{
			if( type == PTT_STRING && dta.wcharPtr ) {
				delete dta.wcharPtr;
			}
		}
		// typified init functions
		void dta_init_token( uint32_t tid ) 
			{ 
				claer();
				type = PTT_TOKEN;
				dta.numInt = 0;
				dta.token.id = tid;
			}
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

		inline bool isNull() 		    const{ return (type == PTT_NULL); }
	
		void print( std::ostream& fp );
		
		ParsedToken( size_t id ) : 
			type(PTT_TOKEN)
		{ dta_init_token( id ); }

		ParsedToken( ) : 
			type(PTT_NULL)
		{ dta.wcharPtr = 0; }
	}; // ParsedToken class ends
	
	inline bool operator <( const ParsedToken& l, const ParsedToken& r )
		{ return ParsedToken::comp_less()( l, r ); }
	inline bool operator ==( const ParsedToken& l, const ParsedToken& r )
		{ return ParsedToken::comp_eq()( l, r ); }

	
	/// converts input wstring-s into tokens
	/// using te intern wide strings from the pool
	class Tokenizer {
		ay::WStringPool& sPool;

		wchar* getWCar( const ParsedToken&  );
		int printTok( std::ostream& , const PrasedToken& );

		/// tries to resolve token, otherwise adds it. 
		/// it the last parameter is NULL - automatically figures the type
		/// otherwise performs conversion (INT, DOUBLE) without storing anything 
		/// 
		int makeTok( ParsedToken&, const std::wstring&, ParsedToken::PTType= ParsedToken::PTT_NULL );
		/// same as makeTok except doesnt create whenever a token cant be resolved
		int getTok( ParsedToken&, const std::wstring&, ParsedToken::PTType= ParsedToken::PTT_NULL );
		Tokenizer( ay::WStringPool& pool ) : 
			sPool(pool) {}
	};
	
} // barzer namespce ends
