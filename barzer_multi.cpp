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
    // splits the bufer into 2 new ones. before and after <ID> in u="<ID>"
    // cuts out multi="<anything>"
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

    auto parse = [&children, this](std::ostream &os, size_t i) {
        std::string w_in;
        gen_input(children[i], w_in);
        RequestEnvironment w_reqEnv(os, w_in.c_str(), w_in.size());
        BarzerRequestParser rp(gpools, os);
        rp.getBarz().setServerReqEnv( &w_reqEnv );
        rp.parse(w_reqEnv.buf, w_reqEnv.len);
    };

    uint32_t wlen = children.size() - 1;
    // makes a vector of N-1 streams for workers
    std::vector<std::shared_ptr<std::ostringstream> > outputs;
    outputs.reserve(wlen);
    for (size_t i = 0; i < wlen; ++i) { // can someone please shoot me. Or at least give me a working vector<ostream>
        outputs.push_back(std::make_shared<std::ostringstream>());
    }

    std::atomic<int> left(wlen);
    for (size_t i = 1; i < wlen; ++i) { // send out the clowns
        wg.run_task([this, i, &left, parse, &outputs]() {
            parse(*outputs[i], i);
            --left;
        });
    }

    // writing right into the output stream for the last one
    std::ostream &os = reqEnv.outStream;
    parse(os, wlen);

    while (left > 0)
        ; // wait for all workers to finish

    // writing out workers results
    for (auto &ss : outputs) {
        os << ss->rdbuf();
    }


}
}
