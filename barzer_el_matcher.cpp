#include <barzer_el_chain.h>
#include <barzer_el_matcher.h>
#include <barzer_el_btnd.h>
#include <barzer_universe.h>
#include <barzer_barz.h>
#include <ay_logger.h>

namespace barzer {

bool BarzelMatcher::match( RewriteUnit& ru, BarzelBeadChain& beads )
{
	#warning BarzelMatcher::match unimplemented
	return false;
}

bool BarzelMatcher::evaluateRewriteUnit( const RewriteUnit&, const BarzelBeadChain& ) const
{
	#warning BarzelMatcher::evaluateRewriteUnit
	return true;
}

/// 
int BarzelMatcher::rewriteUnit( const RewriteUnit&, BarzelBeadChain& )
{
	#warning BarzelMatcher::evaluateRewriteUnit
	return 0;
}
int BarzelMatcher::matchAndRewrite( Barz& barz )
{
	AYTRACE( "BarzelMatcher::matchAndRewrite unimplemented" );
	clear();

	BarzelBeadChain beads = barz.getBeads();

	int rewrCount = 0, matchCount = 0;


	for( ; ; ++ matchCount ) {
		if( matchCount> MAX_MATCH_COUNT ) {
			d_err.setCode( Error::ERR_CRCBRK_MATCH );
			AYLOG(ERROR) << "Match count circuit breaker hit .." << std::endl;
			break;
		}

		RewriteUnit rewrUnit;
		if( !match( rewrUnit, beads) ) {
			break; // nothing matched
		}
		/// here we can add some debugging where we would log rewrUnit.first (matching info)

		if( evaluateRewriteUnit( rewrUnit, beads ) ) {
			if( rewrCount > MAX_REWRITE_COUNT ) {
				d_err.setCode( Error::ERR_CRCBRK_MATCH );
				AYLOG(ERROR) << "Rewrite count circuit breaker hit .." << std::endl;
				break;
			}
			int rewrRc = rewriteUnit( rewrUnit, beads );
			if( rewrRc ) { // should never be the case
				d_err.setFlag( Error::EF_RWRFAILS );
				AYLOG(ERROR) << "rewrite failed"  << std::endl;
			} else
				++rewrCount;
		} else 
			d_exclList.addExclusion( rewrUnit.first );
		
	}
	return rewrCount;
}

} // namespace barzer ends 
