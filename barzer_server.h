#ifndef BARZER_SERVER_H
#define BARZER_SERVER_H

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

#include <ay/ay_headers.h>
#include <ay/ay_logger.h>

#include <barzer_universe.h>
#include <barzer_barz.h>
#include <barzer_parse.h>
#include <barzer_server_request.h>

namespace barzer {
int run_server(int port);
}

#endif

