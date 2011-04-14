#include <barzer_el_chain.h>

namespace barzer {

BarzelBead::BarzelBead( const CToken& ct ) : tokenSpan(0), tokenCount(0) 
{
	switch( ct.getCTokenClass() ) {

	}
}

}
