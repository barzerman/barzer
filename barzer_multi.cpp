/*
 * barzer_multi.cpp
 *
 *  Created on: Nov 3, 2014
 *      Author: polter
 */



#include "barzer_multi.h"
#include <stdlib.h>
namespace barzer {

namespace {
WorkerGroup& getWorkerGroup() {
    static WorkerGroup wg(6);
    return wg;
}
}

MultiQueryHandler::MultiQueryHandler(GlobalPools& gp, RequestEnvironment& re)
    : gpools(gp), reqEnv(re), wg(getWorkerGroup()) {
    const char * buf = re.buf;
    char *bs = bufStart = new char[re.len];
    while(*buf) {
        switch(*buf) {
        case 'u':
            if (buf[1] == '=' || isspace(buf[1])) {
                do {
                    *bs++ = *buf;
                } while (*buf++ != '"');
                *bs++ = '\0';
                bufEnd = bs;
                orig_uid = (uint32_t)strtoul(buf, NULL, 10);
                buf = strchr(buf, '"');
            }
            break;
        case 'm':
            if (strncmp(buf+1, "ulti", sizeof("ulti"))
                    && (*(buf+5) == '=' || isspace(*(buf+5)))) {
                while(*buf++ != '"')
                    ;
                while(*buf++ != '"')
                    ;
            }
            break;
        }
        *bs++ = *buf++;
    }
}


void MultiQueryHandler::process() {


}
}
