#include <barzer_universe.h>
#include <barzer_bzspell.h>
#include <ay/ay_choose.h>

namespace barzer {
typedef std::vector<char> charvec;
typedef charvec::const_iterator charvec_ci;

bool BZSpell::isWordValidInUniverse( const char* word ) const
{
	uint32_t strId = d_universe.getGlobalPools().string_getId( word );
	return ( strId == 0xffffffff ? false: (d_wordinfoMap.find( strId ) != d_wordinfoMap.end()) ) ;
}
void BZSpell::addExtraWordToDictionary( uint32_t strId, uint32_t frequency )
{
	BZSWordInfo& wi = d_wordinfoMap[ strId ];
	if( wi.upgradePriority( BZSWordInfo::WPRI_USER_EXTRA ) )
		wi.setFrequency( frequency );
}

bool BZSpell::getWordinfo( uint32_t strId, WordInfoAndDepth& wid ) const
{
	strid_wordinfo_hmap::const_iterator i = d_wordinfoMap.find( strId );
	if( i == d_wordinfoMap.end() ) {
		++(wid.second);
		return ( d_secondarySpellchecker ? d_secondarySpellchecker->getWordinfo(strId,wid): false ) ;
	} else {
		wid.first = &(i->second);
		return true;
	}
}

uint32_t BZSpell::getWordinfoByWord( const char* word, WordInfoAndDepth& wid ) const 
{
	uint32_t strId = d_universe.getGlobalPools().string_getId( word );

	if( strId == 0xffffffff ) 
		return 0xffffffff;
	else {
		wid.second = 0;
		return ( getWordinfo( strId, wid ) ? strId: 0xffffffff );
	}	
}

namespace {

struct CorrectCallback {
	const BZSpell& d_bzSpell;
	typedef BZSpell::WordInfoAndDepth CorrectionQualityInfo;

	CorrectionQualityInfo d_bestMatch;
	uint32_t d_bestStrId;
	
	static bool widLess( const CorrectionQualityInfo& l, const CorrectionQualityInfo& r ) 
	{
		return ay::range_comp().less_than( 
			l.second, *(l.first),
			r.second, *(r.first)
		);
	}
	CorrectCallback( const BZSpell& bzs ) : 
		d_bzSpell(bzs) ,
		d_bestMatch(0,0),
		d_bestStrId(0xffffffff)
	{}

	int operator()( charvec_ci fromI, charvec_ci toI )
	{
		charvec v( fromI, toI );
		v.push_back(0);
		const char* str = &(v[0]);

		CorrectionQualityInfo wid(0, 0);
		
		uint32_t strId = d_bzSpell.getWordinfoByWord( str, wid);
		if( (0xffffffff != strId) && widLess(d_bestMatch,wid) ) {
			d_bestMatch= wid;
			d_bestStrId= strId;
		}

		return 0;
	}
	
	uint32_t getBestStrId() const { return d_bestStrId; }
	const CorrectionQualityInfo& getBestMatchInfo() const { return  d_bestMatch; }
}; 

} // anonymous namespace 

/// when fails 0xffffffff is returned 
uint32_t BZSpell::getSpellCorrection( const char* str ) const
{
	/// for ascii corrector
	if( isAscii() ) {
		size_t str_len = strlen( str );
		if( str_len > d_minWordLengthToCorrect ) {
			CorrectCallback cb( *this );	
			ay::choose_n<char, CorrectCallback > variator( cb, str_len-1, str_len-1 );
			variator( str, str+str_len );
			return cb.getBestStrId();
		}
	} 
	return 0xffffffff;
}
namespace ascii {

inline bool is_vowel( char c )
{
	return(  c == 'a' || c =='o' || c=='i' || c=='e' || c=='u' || c=='y' );
}
inline bool is_consonant( char c )
{
	return( !is_vowel(c) );
}

// XXtch --> XXXtches
// XXx --> XXxes
// Xvse -->  XXvses
// XXss ---> XXsses
// Xse ---> XXses
// X ---> Xs

// s_len MUST be == strlen(s)
struct s2s{
	const char* fromS;
	const char* toS;
};
inline bool operator<( const s2s& l, const s2s& r ) {
	return ( strcmp(l.fromS, r.fromS ) < 0 );
}

bool stem_depluralize( std::string& out, const char* s, size_t s_len )
{
	if( s_len > 5 ) {
		if( !strcmp( s+s_len-4, "ches" ) ) {
			out.assign( s, s_len-2 );
			return true;
		}
	} else if( s_len > 4 ) {
		const char* s4 = s+s_len-4;
		const char* s3 = (s4+1);
		if( !strcmp(s3,"ses") ) {
			if(*s4=='s') {    	  // asses 
				out.assign( s, s_len-2 );
				return true;
			} 
			if(  is_vowel(*s4)) { // vases, blouses
				out.assign( s, s_len-1 );
				return true;
			} else { 			  // putses
				out.assign( s, s_len-2 );
				return true;
			}
		}
	} else if( s_len > 3 ) {
		const char* s3 = s+s_len-3;
		if( s3[1] == 'e' && s3[2] == 's' && s3[0] =='x' ) { // xes
				out.assign( s, s_len-2 );
				return true;
		} else if( s[1] == 'u' && s[2] == 's' ) {
			return false;
		} else if( s[1] == 'e' && s[2] == 's' ) {
			if( is_vowel(s[0])) {
				out.assign( s, s_len-1 );
			} else {
				out.assign( s, s_len-2 );
			}
			return true;
		}
	} else if( s_len > 2 ) {
		const char* s2 = s+s_len-2;
		if( s2[0] =='e' && s2[1] == 's' ) {
		}
		if( s[ s_len-1 ] == 's' ) {
			out.assign( s, s_len-1 );
			return true;
		}
			
	} else
		return false;
	
	#define D(x,y) {#x,#y}
	static const s2s exception[] = {
			D(alumni,alumnus),
			D(brethren,brother),
			D(calves,calf),
			D(censuses,census),
			D(children,child),
			D(clothes,cloth),
			D(corpora,corpus),
			D(dice,die),
			D(drwarves,dwarf),
			D(dutchmen,dutchman),
			D(englishmen,englishman),
			D(feet,foot),
			D(fishes,fish),
			D(foci,focus),
			D(frenchmen,frenchman),
			D(geese,goose),
			D(genera,genus),
			D(halves,half),
			D(hooves,hoof),
			D(irishmen,irishman),
			D(knives,knife),
			D(lice,louse),
			D(lives,life),
			D(loaves,loaf),
			D(louse,lice),
			D(men,man),
			D(mice,mouse),
			D(mouse,mice),
			D(passersby,passerby),
			D(pennies,penny),
			D(pense,penny),
			D(people,person),
			D(phalanges,phalanx),
			D(prospectuses,prospectus),
			D(radii,radius),
			D(scotsmen,scotsman),
			D(swine,pig),
			D(syllabi,syllabus),
			D(teeth,tooth),
			D(viscera,viscus),
			D(wives,wife),
			D(women,woman)
	};
	#undef D
	s2s chablon = {s,0};
	const s2s* exception_end = exception + ARR_SZ(exception);
	const s2s* ew = std::lower_bound( exception, exception_end, chablon );
	if( ew != exception_end && ew ) {
		out.assign( ew->toS );
		return true;
	}
	return false;
}

}

// this implements a simple single suffix stem (not a complete linguistic one)
uint32_t BZSpell::getStem( std::string& out, const char* s ) const
{
	if( isAscii() ) {
		size_t s_len = strlen(s);
		if( s_len > d_minWordLengthToCorrect ) {
			std::string out;
			if( ascii::stem_depluralize( out, s, s_len ) ) {
				uint32_t strId = d_universe.getGlobalPools().string_getId( out.c_str() );
				if( strId == 0xffffffff ) {
					if( d_secondarySpellchecker )
						return( d_secondarySpellchecker->getUniverse().getGlobalPools().string_getId( out.c_str() ) );
				} else {
					return strId;
				}
			}
		}
	}
	return 0xffffffff;
}

namespace {

struct WPCallback {
	BZSpell& bzs;
	uint32_t fullStrId;
	size_t   varCount; 
	WPCallback( BZSpell& b, uint32_t l2  ) : bzs(b), fullStrId(l2), varCount(0)  {}

	int operator()( charvec_ci fromI, charvec_ci toI )
	{
		charvec v( fromI, toI );
		v.push_back(0);
		GlobalPools& gp = bzs.getUniverse().getGlobalPools();
		const char* str = &(v[0]);

		uint32_t strId = gp.string_intern( str );

		bzs.addWordToLinkedWordsMap( strId, fullStrId );
		++varCount;
		return 0;
	}
};

}

size_t BZSpell::produceWordVariants( uint32_t strId )
{
	/// for ascii 
	const char* str = d_universe.getGlobalPools().string_resolve( strId );
	if( !str ) 
		return 0;

	int stringClass = ay::strparse::is_string_ascii(str) ;
	if( stringClass == 0 ) {
		size_t str_len = strlen( str );
	
		if( str_len > d_minWordLengthToCorrect ) {
			WPCallback cb( *this, strId );	
			ay::choose_n<char, WPCallback > variator( cb, str_len-1, str_len-1 );
			variator( str, str+str_len );
			return cb.varCount;
		}
	} 
	return 0;
}

BZSpell::BZSpell( StoredUniverse& uni ) : 
	d_universe( uni ),
	d_charSize(1) ,
	d_minWordLengthToCorrect( d_charSize* 3 )
{}

size_t BZSpell::loadExtra( const char* fileName )
{
	if( !fileName ) 
		return 0;
	FILE * fp = fopen( fileName, "r" );
	if( !fp ) {
		std::cerr << "cant open file \"" << fileName << "\" for reading\n";
		return 0;
	}

	char buf[ 256 ];
	size_t numWords = 0;

	while( fgets( buf, sizeof(buf)-1, fp ) ) {
		uint32_t freq = 0;
		size_t buf_last = strlen(buf)-1;
		if( buf[buf_last] == '\n' ) 
			buf[ buf_last ] = 0;

		char * sep = ay::strparse::find_separator( buf, "|,;" );
		if( sep ) {
			*sep = 0;
			++sep;
			freq = (uint32_t)atoi( sep );
		}
		uint32_t strId = d_universe.internString( buf, true, freq );
		addExtraWordToDictionary( strId, freq );
		++numWords;
	}
	return numWords;
}

std::ostream& BZSpell::printStats( std::ostream& fp ) const
{
	fp << d_wordinfoMap.size() << " actual words, " << d_linkedWordsMap.size() << " partials\n";

	for( strid_wordinfo_hmap::const_iterator i = d_wordinfoMap.begin(); i != d_wordinfoMap.end(); ++i ) {
		const char* str = d_universe.getGlobalPools().string_resolve( i->first );
		std::cerr << "FULL:" << ( str? str: "<null>" ) << "\n";
	}
	for( strid_evovec_hmap::const_iterator i = d_linkedWordsMap.begin(); i != d_linkedWordsMap.end(); ++i ) {
		const char* str = d_universe.getGlobalPools().string_resolve( i->first );
		std::cerr << "PARTIAL:" << ( str? str: "<null>" ) << "\n";
	}
	return fp;
}
size_t BZSpell::init( const StoredUniverse* secondaryUniverse ) 
{
	if( secondaryUniverse ) {
		d_secondarySpellchecker = secondaryUniverse->getBZSpell();
	} else {
		const UniverseTrieCluster::BELTrieList& trieList = d_universe.getTrieList(); 
		for( UniverseTrieCluster::BELTrieList::const_iterator t = trieList.begin(); t!= trieList.end(); ++t ) {
			const strid_to_triewordinfo_map& wiMap = (*t)->getWordInfoMap();
			for( strid_to_triewordinfo_map::const_iterator w = wiMap.begin(); w != wiMap.end(); ++w ) {
				const TrieWordInfo& wordInfo = w->second;
				uint32_t strId = w->first;
	
				BZSWordInfo& wi = d_wordinfoMap[ strId ];
				
				if( wi.upgradePriority( (*t)->getSpellPriority()) )
					wi.setFrequency( wordInfo.wordCount );
			}
		}
	}

	/// at this point d_wordinfoMap has an entry for every word 
	/// now we will generate edit distance 1 variants for each word 
	for( strid_wordinfo_hmap::const_iterator wi = d_wordinfoMap.begin(); wi!= d_wordinfoMap.end(); ++wi ) {
		uint32_t strId = wi->first;

		produceWordVariants( strId );
	}
	return d_linkedWordsMap.size();
}

} // namespace barzer
