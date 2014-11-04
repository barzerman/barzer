/*
 * barzer_multi.h
 *
 *  Created on: Nov 3, 2014
 *      Author: polter
 */

#pragma once
#include "barzer_universe.h"
#include "barzer_server.h"
#include <ay/ay_wgroup.h>

namespace barzer {
class MultiQueryHandler {
    GlobalPools &gpools;
    RequestEnvironment &reqEnv;
    uint32_t orig_uid;
    WorkerGroup &wg;
    char *bufStart;
    char *bufEnd;
public:
    MultiQueryHandler(GlobalPools&, RequestEnvironment&);
    void process();
    void gen_input(uint32_t user_id, std::string&);
};
}
