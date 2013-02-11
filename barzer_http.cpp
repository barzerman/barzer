#include <barzer_http.h>
#include <barzer_server_request.h>

extern "C" {
#include <mongoose/mongoose.h>

static int begin_request_handler(struct mg_connection *conn) 
{
    const struct mg_request_info *request_info = mg_get_request_info(conn);
    if( !request_info || !request_info->uri || !request_info->query_string )
        return 1;
    
    const barzer::BarzerHttpServer& httpServ = barzer::BarzerHttpServer::instance();
    barzer::GlobalPools& gp = httpServ.gp; /// gp must not be changed - constant onsistency is hard to achieve but gp is const
    
    std::stringstream outSstr;
    barzer::BarzerRequestParser reqParser(gp,outSstr);
    if( !reqParser.initFromUri( request_info->uri, strlen(request_info->uri), request_info->query_string, strlen(request_info->query_string) ) ) 
        reqParser.parse();

    // Send HTTP reply to the client
    std::string contentStr = outSstr.str();
    mg_printf(conn,
        "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"        // Always set Content-Length
            "\r\n"
            "%s",
            ( reqParser.ret == barzer::BarzerRequestParser::XML_TYPE ? "text/xml" :
                (reqParser.ret == barzer::BarzerRequestParser::JSON_TYPE ? "application/json" : "text/plain")
            ), 

        contentStr.length(), contentStr.c_str());

    // Returning non-zero tells mongoose that our function has replied to
    // the client, and mongoose should not send client any more data.
    return 1;
}

}

namespace barzer {

namespace {

BarzerHttpServer* g_httpServer = 0;
}

void BarzerHttpServer::destroyInstance() 
    { delete g_httpServer; }
BarzerHttpServer& BarzerHttpServer::instance() { return *g_httpServer; }

BarzerHttpServer& BarzerHttpServer::mkInstance(GlobalPools& g)
{
    g_httpServer = new BarzerHttpServer(g);
    return *g_httpServer;
}

int BarzerHttpServer::run(const ay::CommandLineArgs& cmd, int argc, char* argv[] )
{
    struct mg_context *ctx;
    struct mg_callbacks callbacks;

    // List of options. Last element must be NULL.
    const char *options[] = {"listening_ports", "8080", NULL};

    std::cerr << "RUNNING HTTP SERVER \n"; 
    // Prepare callbacks structure. We have only one callback, the rest are NULL.
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.begin_request = begin_request_handler;
    
    // Start the web server.
    ctx = mg_start(&callbacks, NULL, options);
    
    while( getchar() != '.' ) ;
    // Stop the server.
    mg_stop(ctx);

    return 0;
}

} // namespace barzer 
