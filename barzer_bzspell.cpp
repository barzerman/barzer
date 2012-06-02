#include <barzer_universe.h>
#include <barzer_bzspell.h>
#include <ay/ay_choose.h>
#include <ay_char.h>
#include <ay_utf8.h>
#include <lg_ru/barzer_ru_lex.h>
#include <lg_ru/barzer_ru_stemmer.h>
#include <lg_en/barzer_en_lex.h>

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
    strid_wordinfo_hmap::iterator wmi = d_wordinfoMap.find( strId );
    if( wmi == d_wordinfoMap.end() ) {
        wmi = d_wordinfoMap.insert( strid_wordinfo_hmap::value_type(
            strId, BZSWordInfo()
        )).first;

        // determine language here (english is always default)
	    const char* str = d_universe.getGlobalPools().string_resolve( strId );
	    if( str ) {
            size_t s_len = strlen(str);
            int16_t lang = Lang::getLang( str, s_len );
	        if( lang != LANG_ENGLISH )
                wmi->second.setLang(lang);
        }

    }
	// BZSWordInfo& wi = d_wordinfoMap[ strId ];
	BZSWordInfo& wi = wmi->second;

	if( wi.upgradePriority( BZSWordInfo::WPRI_USER_EXTRA ) )
		wi.setFrequency( frequency );
}

uint32_t  BZSpell::getBestWord( uint32_t strId, WordInfoAndDepth& wid ) const
{
	{ // checking to see whether strId represents a whole word
		// whole word will always win
		strid_wordinfo_hmap::const_iterator w= d_wordinfoMap.find( strId );
		if( w != d_wordinfoMap.end() )
			return ( wid.first = &(w->second), strId );
	}
	// looking for linked words
	strid_evovec_hmap::const_iterator evi = d_linkedWordsMap.find( strId );
	if( evi !=  d_linkedWordsMap.end() ) {
		const uint32_t * evo_end = evi->second.end_ptr();
		const BZSWordInfo* tmpWordInfo= 0;
		uint32_t bestWordId = 0xffffffff;
		for( const uint32_t* ew = evi->second.begin_ptr(); ew!= evo_end; ++ew ) {
			strid_wordinfo_hmap::const_iterator w = d_wordinfoMap.find( *ew );
			if( w != d_wordinfoMap.end() ) {

				const BZSWordInfo& curInfo = w->second;
				if( !tmpWordInfo || *(tmpWordInfo) < curInfo ) {
					bestWordId = w->first;
					tmpWordInfo = &(w->second);
				}
			}
		}
		if( bestWordId != 0xffffffff ) // should always be the case
			{ return ( wid.first = tmpWordInfo, bestWordId ); }
	}

	++(wid.second);
	return ( d_secondarySpellchecker ? d_secondarySpellchecker->getBestWord(strId,wid): 0xffffffff ) ;
}

uint32_t BZSpell::getBestWordByString( const char* word, WordInfoAndDepth& wid ) const
{
	uint32_t strId = d_universe.getGlobalPools().string_getId( word );

	if( strId == 0xffffffff )
		return 0xffffffff;
	else {
		wid.second = 0;
		return getBestWord( strId, wid ) ;
	}
}

namespace {

struct CorrectCallback {
	const BZSpell& d_bzSpell;
	typedef BZSpell::WordInfoAndDepth CorrectionQualityInfo;

	CorrectionQualityInfo d_bestMatch;
	uint32_t d_bestStrId;
    // original string's length
    size_t   d_str_len;
    charvec  d_charvec2B;

    const BZSpell& bzSpell() const { return d_bzSpell; }
	static bool widLess( const CorrectionQualityInfo& l, const CorrectionQualityInfo& r )
	{
		if( l.second < r.second )
			return true;
		else if( l.second < r.second )
			return false;
		if( !l.first )
			return r.first;
		else if( !r.first )
			return false;
		else
			return ( *(l.first) < *(r.first) );
	}

	CorrectCallback( const BZSpell& bzs, size_t str_len ) :
		d_bzSpell(bzs) ,
		d_bestMatch(0,0),
		d_bestStrId(0xffffffff),
        d_str_len(str_len)
	{}

	void tryUpdateBestMatch( const char* str )
	{
		CorrectionQualityInfo wid(0, 0);
		uint32_t strId = d_bzSpell.getBestWordByString( str, wid);
		if( (0xffffffff != strId) && widLess(d_bestMatch,wid) ) {
            //uint32_t uwId = 0xffffffff;
            //if( d_bzSpell.isUsersWord( uwId, str ) ) {
                d_bestMatch= wid;
                d_bestStrId= strId;
            //}

		}
	}

	int operator() (std::vector<ay::CharUTF8>::const_iterator begin, std::vector<ay::CharUTF8>::const_iterator end)
	{
		tryUpdateBestMatch (ay::StrUTF8 (begin, end).c_str ());
		return 0;
	}

    typedef std::vector< ay::Char2B > char_2b_vec;
	int operator()( char_2b_vec::const_iterator fromI, char_2b_vec::const_iterator toI )
    {
		ay::Char2B::mkCharvec( d_charvec2B, fromI, toI );
		const char* str = &(d_charvec2B[0]);
        /*
        {
            uint32_t id= 0xffffffff;
            if( !d_bzSpell.isUsersWord( id, str ) )
                return 0;
        }
        */

		tryUpdateBestMatch( str );
		return 0;
    }

	int operator()( charvec_ci fromI, charvec_ci toI )
	{
		charvec v( fromI, toI );
		v.push_back(0);
		const char* str = &(v[0]);
        if( d_str_len <5 ) {
            uint32_t id= 0xffffffff;
            if( !d_bzSpell.isUsersWord( id, str ) )
                return 0;
        }

		tryUpdateBestMatch( str );
		return 0;
	}

	uint32_t getBestStrId() const { return d_bestStrId; }
	const CorrectionQualityInfo& getBestMatchInfo() const { return  d_bestMatch; }
};


} // anonymous namespace

namespace ascii {

struct CharPermuter {
	size_t buf_len;
	CorrectCallback& callback;
	char buf[ 128 ];

	CharPermuter( const char* s, CorrectCallback& cb ) : buf_len(strlen(s)), callback(cb)
	{
		strncpy( buf, s, sizeof(buf)-1 );
		buf[ sizeof(buf)-1 ] = 0;
	}
	void doAll() {
		if( buf_len< 3 ) return ;

		char* buf_last = &buf[buf_len-2];
		for( char* s = buf; s <= buf_last; ++s ) {
			std::swap( s[0], s[1] );

            if( buf_len <5 ) {
                uint32_t id= 0xffffffff;
                if( !callback.bzSpell().isUsersWord( id, buf ) ) {
			        std::swap( s[0], s[1] );
                    continue;
                }
            }
			callback.tryUpdateBestMatch( buf );
			std::swap( s[0], s[1] );
		}
	}
};
struct CharPermuter_2B {
	size_t buf_len;
	CorrectCallback& callback;
	char buf[ 128 ];

	CharPermuter_2B( const char* s, CorrectCallback& cb ) : buf_len(strlen(s)), callback(cb)
	{
		strncpy( buf, s, sizeof(buf)-1 );
		buf[ sizeof(buf)-1 ] = 0;
	}
	void doAll() {
		if( buf_len< 6 ) return ;

		char* buf_last = &buf[buf_len-4];
		for( char* s = buf; s <= buf_last; s+=2 ) {
			std::swap( s[0], s[2] );
			std::swap( s[1], s[3] );

            if( buf_len <10 ) {
                uint32_t id= 0xffffffff;
                if( !callback.bzSpell().isUsersWord( id, buf ) ) {
			        std::swap( s[0], s[2] );
			        std::swap( s[1], s[3] );
                    continue;
                }
            }
			callback.tryUpdateBestMatch( buf );
			std::swap( s[0], s[2] );
			std::swap( s[1], s[3] );
		}
	}
};
struct CharPermuter_Unicode
{
	ay::StrUTF8 m_str;
	CorrectCallback& m_cb;

	CharPermuter_Unicode (const char *s, CorrectCallback& cb)
	: m_str (s)
	, m_cb (cb)
	{
	}

	CharPermuter_Unicode (const ay::StrUTF8& uni, CorrectCallback& cb)
	: m_str (uni)
	, m_cb (cb)
	{
	}

	void doAll ()
	{
		if (m_str.size () < 3)
			return;

		for (ay::StrUTF8::iterator i = m_str.begin (), end = m_str.end () - 1; i != end; ++i)
		{
			m_str.swap (i, i + 1);

			const char *newBuf = m_str.c_str ();
			if (m_str.size () < 5)
			{
				uint32_t id = 0xffffffff;
				if (!m_cb.bzSpell ().isUsersWord (id, newBuf))
				{
					m_str.swap (i, i + 1);
					continue;
				}
			}
			m_cb.tryUpdateBestMatch (newBuf);
			m_str.swap (i, i + 1);
		}
	}
};

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
inline bool is_pure_vowel( char c )
    { return( c=='a' || c =='u' || c == 'e' || c =='o' || c == 'i' ); }

bool stem_detense( std::string& out, const char* s, size_t s_len )
{
    if( s_len > 4 ) {
        #define D(x,y) {#x,#y}
        static const s2s exception[] = {
D(adhered,adhere),
D(cancelled,cancel),
D(compelled,compel),
D(compelled,compel),
D(cupelled,cupel),
D(dispelled,dispel),
D(expelled,expel),
D(gospelled,gospel),
D(impelled,impel),
D(lapelled,lapel),
D(propelled,propel),
D(rappelled,rapel),
D(reexpelled,reexpel),
D(repelled,repel),
D(repelled,repel),
D(respelled,repel),
D(revered,revere),
D(stirred,stir)

        };
        #undef D
	    s2s chablon = {s,0};
        const s2s* exception_end = exception + ARR_SZ(exception);
        const s2s* ew = std::lower_bound( exception, exception_end, chablon );
        if( ew && ew != exception_end && !strcmp(ew->fromS,s) ) {
            out.assign( ew->toS );
            return true;
        }

		const char* s4 = s+s_len-4;
		const char* s3 = (s4+1);
		const char* s2 = (s3+1);

        if( s_len > 6 && s3[0] == 'i' && s3[1] == 'n' && s3[2] == 'g' ) { // ing
            /// will deal with ING later
        } else if( s_len >4 ) {
            if( s2[0] == 'e' && s2[1] == 'd' ) {
                if( *s3 == 'i' ) {
                    out.assign( s, s_len-2 );
                    *(out.rbegin())='y';
                    return true;
                } else if( *s3 == 'u' ) {
                    return( out.assign( s, s_len-1 ), true );
                }
                if( is_pure_vowel(*s3) ) // if its VowelED - no stripping
                    return false;
                if( is_vowel(*s4) && (*s3== 'x' || *s3 == 'y' || *s3 =='r' || *s3=='l' || *s3=='w')  )
                    return( out.assign( s, s_len-2 ), true );

                if( is_pure_vowel(*s4))  { // VCed --> VCe
                    if( s_len > 5 ) {
                        const char* s5 = s4-1;
                        if( is_pure_vowel(*s5) ) // VVCed --> VVC
                            return( out.assign( s, s_len-2 ), true );
                        if( *s5 == 'h' && *s4 == 'e' && *s3 =='n')
                            return( out.assign( s, s_len-2 ), true );
                    }
                    return( out.assign( s, s_len-1 ), true );
                }
                // s4 definitely not a vowel
                if( *s4 == *s3 && *s3 != 'l' ) { /// double consonant followed by ED
                    if( s_len > 5 ) // we strip ed and the preceding consonant
                        return( out.assign( s, s_len-3 ), true );
                } else {
                    if( s_len > 5 ) {
                        if( *s3 == 'g' && *s4 == 'n' && *(s4-1) == 'a' ) { // anged
                            if( *(s4-2) == 'r' ) // ranged
                                return( out.assign( s, s_len-1 ), true );
                        }
                    }
                }

                return( out.assign( s, s_len-2 ), true );
            }
        }
    }
    return false;
}

bool stem_depluralize( std::string& out, const char* s, size_t s_len )
{
    const char* exceptionStem = ascii::english_exception_depluralize(s,s_len);
    if( exceptionStem )
        return( out.assign( exceptionStem ), true );

	if( s_len > 5 ) {
		const char* s4 = s+s_len-4;
		const char* s3 = (s4+1);
        if( s3[1] =='e' && s3[2] =='s' ) { // es
		    if( s4[0] =='c' && s4[1] == 'h'  ) { /// XXXches ---> XXXch
			    out.assign( s, s_len-2 );
			    return true;
		    } else if( s3[0] =='i' && s3[1] =='e' ) { ///XXXies -->> XXXy
                out.assign( s, s_len-2 );
                *(out.rbegin()) = 'y';
                return true;
            } else {
                out.assign( s, s_len-1 );
                return true;
            }
        } else if( s3[2] == 's' ) {
            out.assign( s, s_len-1 );
            return true;
        }
	} else if( s_len > 4 ) {
		const char* s4 = s+s_len-4;
		const char* s3 = (s4+1);
		if( s3[1] == 'e' && s3[2] == 's' ) {
			if(*s4=='s' || *s3 == 'x') {    	  // asses
				out.assign( s, s_len-2 );
				return true;
			}
			if(  is_vowel(*s3)) { // shoes
                return( out.assign( s, s_len-1 ), true );
            } else if(  is_vowel(*s4)) { // vases, blouses
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
		if( s2[1] == 's' ) {
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
	if( ew && ew != exception_end && !strcmp(ew->fromS,s) ) {
		out.assign( ew->toS );
		return true;
	} else {
        // picking up irregular pluralizations and such
		const char* s2 = s+s_len-2;
        if( s_len > 4 ) {
            if( s2[0] == 'e' && s2[1] == 's' )
                return ( out.assign( s, s_len-1 ), true );
            else if( (s2[0] != 's') &&  s2[1] == 's' )
                return ( out.assign( s, s_len-1 ), true );
        } else if( s_len == 4 ) {
            const char c2 = s2[0];
            if( s2[1] == 's' && c2 != 's' ) {
                return ( out.assign( s, s_len-1 ), true );
            }
        }
    }
	return false;
}

} // ascii namespace ends

int BZSpell::isUsersWordById( uint32_t strId ) const
{
	strid_wordinfo_hmap::const_iterator i =d_wordinfoMap.find( strId );
	if( i == d_wordinfoMap.end() ) {
		if( d_secondarySpellchecker ) {
			int rc = d_secondarySpellchecker->isUsersWordById( strId );
			return ( rc ? rc+1 : 0 );
		} else
			return 0;
	} else
		return 1;
}

int BZSpell::isUsersWord( uint32_t& strId, const char* word ) const
{
	strId = d_universe.getGlobalPools().string_getId( word ) ;
	return( strId == 0xffffffff ? 0 : isUsersWordById(strId) );
}

/// when fails 0xffffffff is returned
uint32_t BZSpell::getSpellCorrection( const char* str, bool doStemCorrect, int lang ) const
{
	/// for ascii corrector
    size_t s_len = strlen(str);
    if( lang == LANG_UNKNOWN )
        lang = Lang::getLang( str, s_len );

    size_t str_len = strlen( str );
	if( lang == LANG_ENGLISH) {

        if( str_len>= MAX_WORD_LEN )
            return 0xffffffff;

        enum { SHORT_WORD_LEN = 4 };

		if( str_len >= d_minWordLengthToCorrect ) {
			CorrectCallback cb( *this, str_len );
			cb.tryUpdateBestMatch( str );

            if( str_len> d_minWordLengthToCorrect ) {
			    ay::choose_n<char, CorrectCallback > variator( cb, str_len-1, str_len-1 );
			    variator( str, str+str_len );
            }

			ascii::CharPermuter permuter( str, cb );
			permuter.doAll();
			return cb.getBestStrId();
		}
	} else if( Lang::isTwoByteLang(lang)) { // 2 byte char language spell correct
        /// includes russian
        std::string stemStr;

		if( str_len >= 2*d_minWordLengthToCorrect ) {
            if( doStemCorrect ) {
                /// first we try to do trivial (close) stem correction
                if( stem( stemStr, str, lang ) ) {
                    uint32_t stemStrId = d_universe.getGlobalPools().string_getId(stemStr.c_str());
                    if( stemStrId == 0xffffffff ) {
                        doStemCorrect = false;
                    } else {
                        //std::cerr << "SHIT " << stemStr << ":" << stemStrId << std::endl;
                        /// if stem is less than 3 chars shorter than the original we are going to try
                        /// stem correction before any other types of correction
                        if( str_len > stemStr.length() && (str_len-stemStr.length()<3*2) ) {
                            uint32_t correctedId = get2ByteLangStemCorrection( lang, str, doStemCorrect, stemStr.c_str() );
                            if( correctedId != 0xffffffff )
                                return correctedId;
                            else
                                doStemCorrect= false;
                        }
                    }
                }
            }

			CorrectCallback cb( *this, str_len );
			cb.tryUpdateBestMatch( str );

            if( str_len> d_minWordLengthToCorrect ) {
                size_t numChar = str_len/2;
			    ay::choose_n<ay::Char2B, CorrectCallback > variator( cb, numChar-1, numChar-1 );
			    variator( ay::Char2B_iterator(str), ay::Char2B_iterator(str+str_len) );
            }

			ascii::CharPermuter_2B permuter( str, cb );
			permuter.doAll();
			uint32_t retStrId =  cb.getBestStrId();
            if( retStrId != 0xffffffff && (doStemCorrect|| !isPureStem(retStrId)) )
                return retStrId;
            else if( doStemCorrect )
                return get2ByteLangStemCorrection( lang, str, doStemCorrect );
		}
    }
    else
	{
		ay::StrUTF8 strUtf8 (str);
		const size_t uniSize = strUtf8.size ();
		if (uniSize < d_minWordLengthToCorrect)
			return 0xffffffff;

		if (doStemCorrect)
		{
			// TODO
		}

		CorrectCallback cb (*this, strUtf8.size ());
		cb.tryUpdateBestMatch (str);

		if (uniSize > d_minWordLengthToCorrect)
		{
			ay::choose_n<ay::CharUTF8, CorrectCallback> variator (cb, uniSize - 1, uniSize + 1);
			variator (strUtf8.begin (), strUtf8.end ());
		}

		ascii::CharPermuter_Unicode permuter (strUtf8, cb);
		permuter.doAll ();
		return cb.getBestStrId ();
	}
	return 0xffffffff;
}

uint32_t BZSpell::purePermuteCorrect(const char* s, size_t s_len )  const
{
    CorrectCallback cb( *this, s_len );
    cb.tryUpdateBestMatch( s );

    if( s_len> d_minWordLengthToCorrect ) {
        size_t numChar = s_len/2;
        ay::choose_n<ay::Char2B, CorrectCallback > variator( cb, numChar-1, numChar-1 );
        variator( ay::Char2B_iterator(s), ay::Char2B_iterator(s+s_len) );
    }

    ascii::CharPermuter_2B permuter( s, cb );
    permuter.doAll();
    uint32_t retStrId =  cb.getBestStrId();
    return retStrId;
}

uint32_t BZSpell::getUtf8LangStemCorrection( int lang, const char* str, bool doStemCorect, const char* extNorm ) const
{
    #warning NEED to implement getUtf8LangStemCorrection
    return 0xffffffff;
}
uint32_t BZSpell::get2ByteLangStemCorrection( int lang, const char* str, bool doStemCorect, const char* extNorm ) const
{
    size_t s_len = 0;
    if(lang == LANG_RUSSIAN) {
        std::string norm;
        if( extNorm )
            norm.assign(extNorm);

        if( norm.length() || Russian::normalize( norm,str,&s_len) ) { // successfully normalized
            uint32_t strId = 0xffffffff;
            if( isUsersWord(strId,norm.c_str()) )
                return strId;

            const strIds_set*  stridSet = d_universe.getGlobalPools().getStemSrcs(strId);
            if( stridSet ) {
                const BZSWordInfo* bestWTI = 0;
                uint32_t bestStringId = 0xffffffff;
                for( strIds_set::const_iterator i = stridSet->begin(); i!= stridSet->end(); ++i ) {
                    uint32_t protoStrId = *i;
                    if( isUsersWordById(protoStrId) ) {
                        const BZSWordInfo* wip = getWordInfo(protoStrId);

                        if( bestStringId == 0xffffffff ) {
                            bestStringId = protoStrId;
                        }
                        if( wip ) {
                            if( !bestWTI || *bestWTI< *wip ) {
                                bestWTI= wip;
                                bestStringId = protoStrId;
                                continue;
                            }
                        }
                    }
                }
                if( bestStringId != 0xffffffff )
                    return bestStringId;
            }

            if( norm.length() + 2 < strlen(str) )
                return 0xffffffff;
            /// trying to permute correct stemmed word - stem was successful but result of the stem
            /// isnt a valid word
            strId = purePermuteCorrect( norm.c_str(), norm.length() );
            if( strId != 0xffffffff )
                return strId;
        }
    }
    /// chopping the word down to 3/4 size
    size_t numLetters = s_len/2, minNumLet = ( (numLetters*3)/4 );
    if( numLetters <= 4 || numLetters > 16 /* protecting against stupid attack */)
        return 0xffffffff;
    std::string tmp( str, numLetters*2 );
    for( size_t i = numLetters-1; i>=minNumLet; --i, tmp.resize( i*2 )) {
        uint32_t strId = 0xffffffff;
        if( isUsersWord(strId,tmp.c_str()) && (doStemCorect || !isPureStem(tmp.c_str()) ) )
            return strId;
    }
    return 0xffffffff;
}

bool BZSpell::stem( std::string& out, const char* s, int& lang ) const
{
    size_t s_len = strlen( s );
    if( lang == LANG_UNKNOWN )
        lang = Lang::getLang( s, s_len );
	if( lang == LANG_ENGLISH) {
		size_t s_len = strlen(s);
		if( s_len > d_minWordLengthToCorrect ) {
			if( ascii::stem_depluralize( out, s, s_len ) ) {
				return true;
			} else
			if( ascii::stem_detense( out, s, s_len ) ) {
				return true;
            }
		}
	} else {
        switch(lang) {
        case LANG_RUSSIAN: return Russian_Stemmer::stem( out, s );
        default: return false;
        }
    }
	return false;
}
bool BZSpell::stem( std::string& out, const char* s ) const
{
    int lang = LANG_UNKNOWN;
    return stem( out, s, lang );
}

bool BZSpell::isPureStem( const char* str ) const
{
    const StoredToken* tmpTok = d_universe.getStoredToken( str );
    return ( tmpTok && tmpTok->isStemmed() ) ;
}

bool BZSpell::isPureStem( uint32_t strId ) const
{
    const char* str = d_universe.getGlobalPools().string_resolve( strId );
    return( str ? isPureStem(str) : false );
}

// this implements a simple single suffix stem (not a complete linguistic one)
uint32_t BZSpell::getStemCorrection( std::string& out, const char* s, int lang ) const
{
    size_t s_len = strlen( s );

    if( lang == LANG_UNKNOWN )
        lang = Lang::getLang( s, s_len );

	if( lang == LANG_ENGLISH) {
		if( stem( out, s, lang) ) {
			uint32_t strId = d_universe.getGlobalPools().string_getId( out.c_str() );
			if( strId == 0xffffffff ) {
				if( d_secondarySpellchecker )
					return( d_secondarySpellchecker->getUniverse().getGlobalPools().string_getId( out.c_str() ) );
			} else {
				return strId;
			}
		}
	} else {
        uint32_t scorStrId =  ( Lang::isTwoByteLang(lang) ?
            get2ByteLangStemCorrection( lang, s, true ) :
            getUtf8LangStemCorrection( lang, s, true ) );
        if( scorStrId != 0xffffffff && isUsersWordById( scorStrId )) {
            const char* stemCor =  d_universe.getGlobalPools().string_resolve( scorStrId );
            if( stemCor ) {
                return( out.assign( stemCor ) , scorStrId );

            }
        }
    }
	return 0xffffffff;
}
uint32_t BZSpell::getAggressiveStem( std::string& out, const char* s ) const
{
    enum { MIN_STEM_LEN= 3 };
    std::string str(s);

    for( ; str.length() >= MIN_STEM_LEN; str.resize(str.length()-1)  ) {
        uint32_t strId = 0xffffffff;
        if( isUsersWord(strId,str.c_str()) )
            return strId;
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

struct WPCallback_2B {

    typedef std::vector< ay::Char2B > char_2b_vec;

	BZSpell& bzs;
	uint32_t fullStrId;
	size_t   varCount;
    int d_lang;
    charvec d_charV; // to save on allocs

	WPCallback_2B( int lang, BZSpell& b, uint32_t l2  ) : bzs(b), fullStrId(l2), varCount(0), d_lang(0)  {}

	int operator()( char_2b_vec::const_iterator fromI, char_2b_vec::const_iterator toI )
	{
		ay::Char2B::mkCharvec( d_charV, fromI, toI );
		GlobalPools& gp = bzs.getUniverse().getGlobalPools();
		const char* str = &(d_charV[0]);

		uint32_t strId = gp.string_intern( str );

		bzs.addWordToLinkedWordsMap( strId, fullStrId );
		++varCount;
		return 0;
	}
};

struct WPCallback_Unicode
{
	BZSpell& bzs;
	uint32_t fullStrId;
	size_t varCount;

	WPCallback_Unicode (BZSpell& b, uint32_t id)
	: bzs (b)
	, fullStrId (id)
	{
	}

	int operator() (std::vector<ay::CharUTF8>::const_iterator begin, std::vector<ay::CharUTF8>::const_iterator end)
	{
		GlobalPools& gp = bzs.getUniverse ().getGlobalPools ();
		uint32_t strId = gp.string_intern (ay::StrUTF8 (begin, end).c_str ());

		bzs.addWordToLinkedWordsMap (strId, fullStrId);

		++varCount;
		return 0;
	}
};
}

size_t BZSpell::produceWordVariants( uint32_t strId, int lang )
{
	/// for ascii
	const char* str = d_universe.getGlobalPools().string_resolve( strId );
	if( !str )
		return 0;

    d_validTokenMap[ str ] = strId;

    size_t str_len = strlen( str );
	if( lang == LANG_ENGLISH ) {

		if( str_len > d_minWordLengthToCorrect ) {
			WPCallback cb( *this, strId );
			ay::choose_n<char, WPCallback > variator( cb, str_len-1, str_len-1 );
			variator( str, str+str_len );
			return cb.varCount;
		}
	} else if( Lang::isTwoByteLang(lang) ) { /// includes russian
		size_t numChars = str_len/2;
		if( numChars > d_minWordLengthToCorrect ) {
			WPCallback_2B cb( lang, *this, strId );
			ay::choose_n<ay::Char2B, WPCallback_2B > variator( cb, numChars-1, numChars-1 );
			variator( ay::Char2B_iterator(str), ay::Char2B_iterator(str+str_len) );
			return cb.varCount;
		}
	}
	else
	{
		ay::StrUTF8 uni (str);
		const size_t numGlyphs = uni.size ();
		if (numGlyphs <= d_minWordLengthToCorrect)
			return 0;

		WPCallback_Unicode cb (*this, strId);

		ay::choose_n<ay::CharUTF8, WPCallback_Unicode> variator (cb, numGlyphs - 1, numGlyphs - 1);
		variator (uni.begin (), uni.end ());

		return cb.varCount;
	}
	return 0;
}

BZSpell::BZSpell( StoredUniverse& uni ) :
	d_secondarySpellchecker(0),
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
	} else {
        std::cerr << "Reading spellchecker dictionary from " << fileName << "...";
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
    std::cerr << numWords << " words read\n";
	return numWords;
}

std::ostream& BZSpell::printStats( std::ostream& fp ) const
{
	fp << d_wordinfoMap.size() << " actual words, " << d_linkedWordsMap.size() << " partials\n";

	/*
	for( strid_wordinfo_hmap::const_iterator i = d_wordinfoMap.begin(); i != d_wordinfoMap.end(); ++i ) {
		const char* str = d_universe.getGlobalPools().string_resolve( i->first );
		std::cerr << "FULL:" << ( str? str: "<null>" ) << "\n";
	}
	for( strid_evovec_hmap::const_iterator i = d_linkedWordsMap.begin(); i != d_linkedWordsMap.end(); ++i ) {
		const char* str = d_universe.getGlobalPools().string_resolve( i->first );
		std::cerr << "PARTIAL:" << ( str? str: "<null>" ) << "\n";
	}
	*/
	return fp;
}
size_t BZSpell::init( const StoredUniverse* secondaryUniverse )
{
	if( secondaryUniverse ) {
		d_secondarySpellchecker = secondaryUniverse->getBZSpell();
	} else {
		const TheGrammarList& trieList = d_universe.getTrieList();
		for( TheGrammarList::const_iterator t = trieList.begin(); t!= trieList.end(); ++t ) {
			const strid_to_triewordinfo_map& wiMap = t->trie().getWordInfoMap();
			for( strid_to_triewordinfo_map::const_iterator w = wiMap.begin(); w != wiMap.end(); ++w ) {
				const TrieWordInfo& wordInfo = w->second;
				uint32_t strId = w->first;

                if( wordInfo.getWordCount() ) {
                    BZSWordInfo& wi = d_wordinfoMap[ strId ];

                    if( wi.upgradePriority( t->trie().getSpellPriority()) )
                        wi.setFrequency( wordInfo.wordCount );
                }
			}
		}
	}

	/// at this point d_wordinfoMap has an entry for every word
	/// now we will generate edit distance 1 variants for each word
	for( strid_wordinfo_hmap::const_iterator wi = d_wordinfoMap.begin(); wi!= d_wordinfoMap.end(); ++wi ) {
		uint32_t strId = wi->first;

		produceWordVariants( strId, wi->second.getLang() );
	}
	return d_linkedWordsMap.size();
}

} // namespace barzer
