/*
 * barzer_multi.cpp
 *
 *  Created on: Nov 3, 2014
 *      Author: polter
 */



#include "barzer_multi.h"
#include <stdlib.h>
#include <atomic>
#include <sstream>
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

void MultiQueryHandler::gen_input(uint32_t user_id, std::string &out) {
    out.clear();
    out.reserve(reqEnv.len);
    out.append(bufStart);
    out.append(std::to_string(user_id));
    out.append(bufEnd);
}
void MultiQueryHandler::process() {
    std::vector<uint32_t> children = { 200, 201, 202 };
    if (!children.size()) return;
    std::vector<std::ostringstream> outputs;
    uint32_t wlen = children.size() - 1;
    outputs.resize(wlen);

    auto parse = [&children, this](std::ostream &os, uint32_t i) {
        std::string w_in;
        gen_input(children[i], w_in);
        RequestEnvironment w_reqEnv(os, w_in.c_str(), w_in.size());
        BarzerRequestParser rp(gpools, os);
        rp.getBarz().setServerReqEnv( &w_reqEnv );
        rp.parse(w_reqEnv.buf, w_reqEnv.len);
    };

    std::atomic<int> left(wlen);

    for (uint32_t i = 1; i < wlen; ++i) {
        wg.run_task([this, i, &left, parse, &outputs]() {
            parse(outputs[i], i);
            --left;
        });
    }

    std::ostream &os = reqEnv.outStream;
    parse(os, wlen);
    while (left > 0)
        ; // wait for all workers to finish

    for (std::ostringstream &ss : outputs) {
        os << ss.str();
    }


}
}
