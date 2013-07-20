/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_http.h>
#include <barzer_server_request.h>

extern "C" {
#include <mongoose/mongoose.h>

void print_json_error( struct mg_connection *conn, const char* errStr )
{
        std::stringstream  sstr;
        ay::jsonEscape(errStr, sstr<< "{ \"error\" : \"" ) << "\" }";
        std::string s= sstr.str();
        mg_printf(conn,
        "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json; charset=utf-8\r\n"
            "Content-Length: %ld\r\n"        // Always set Content-Length
            "\r\n"
            "%s",

        s.length() , s.c_str() );
}
static int begin_request_handler(struct mg_connection *conn) 
{
    const struct mg_request_info *request_info = mg_get_request_info(conn);
    bool invalidInput = ( !request_info || !request_info->uri || !request_info->query_string );

    const barzer::BarzerHttpServer& httpServ = barzer::BarzerHttpServer::instance();
    barzer::GlobalPools& gp = httpServ.gp; /// gp must not be changed - constant onsistency is hard to achieve but gp is const
    
    std::stringstream outSstr;

    barzer::BarzerRequestParser reqParser(gp,outSstr);
    if( !invalidInput ) {
        std::string uri;
        ay::url_encode( uri, request_info->uri, strlen(request_info->uri) );
        std::string query;
        ay::url_encode( query, request_info->query_string, strlen(request_info->query_string) );
        barzer::QuestionParm qparm;
        if( !reqParser.initFromUri( qparm, uri.c_str(), uri.length(), query.c_str(), query.length() ) )
            reqParser.parse(qparm);
    } else {
        print_json_error(conn, "bad query string" );
        return 1;
    }
    // Send HTTP reply to the client
    std::string contentStr = outSstr.str();
    mg_printf(conn,
        "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"        // Always set Content-Length
            "\r\n"
            "%s",
            reqParser.httpContentTypeString(),
            /*
            ( reqParser.ret == barzer::BarzerRequestParser::XML_TYPE ? "text/xml; charset=utf-8" :
                (reqParser.ret == barzer::BarzerRequestParser::JSON_TYPE ? "application/json; charset=utf-8" : "text/plain; charset=utf-8")
            ), 
            */

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
    const char *options[3] = {"listening_ports", "8080", NULL};
    bool hasArg = false;
    const char* portString = cmd.getArgVal(hasArg, "-p", 0);
    if( hasArg && portString )  {
        options[1] = portString;
    }
    // const char *options[] = {"listening_ports", "8080", NULL};

    std::cerr << "RUNNING HTTP SERVER on port " << options[1] << std::endl; 
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
