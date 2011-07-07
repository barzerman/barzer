#ifndef BARZER_SERVER_H
#define BARZER_SERVER_H

#include <cstdlib>
#include <iostream>

#include <ay/ay_headers.h>
#include <ay/ay_logger.h>

#include <barzer_universe.h>
#include <barzer_barz.h>
#include <barzer_parse.h>
#include <barzer_server_request.h>

namespace barzer {

/// manifest info for a given request
struct RequestEnvironment {
	/// user information
	uint32_t    userId;

	/// request buffer 
	const char* buf;
	size_t      len;

	std::ostream& outStream;

	RequestEnvironment( std::ostream& os ): 
		userId(0),
		buf(0),
		len(0),
		outStream(os)
	{}
	RequestEnvironment( std::ostream& os, const char* b, size_t l ): 
		userId(0),
		buf(b),
		len(l),
		outStream(os)
	{}
};

int run_server(GlobalPools&, uint16_t);

namespace request {

/// standard barze request 
int barze( const GlobalPools&, RequestEnvironment& reqEnv );
int emit( RequestEnvironment& reqEnv );

} // request namespace ends

}

#endif

