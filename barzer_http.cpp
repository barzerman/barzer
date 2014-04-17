/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#include <barzer_http.h>
#include <barzer_server_request.h>

extern "C" {
#include <mongoose/mongoose.h>

int print_json_error( struct mg_connection *conn, const char* errStr )
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
        return 1;
}
int send_json_string( struct mg_connection *conn, const std::string& s )
{
        mg_printf(conn,
        "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json; charset=utf-8\r\n"
            "Content-Length: %ld\r\n"        // Always set Content-Length
            "\r\n"
            "%s",

        s.length() , s.c_str() );
        return 1;
}
/// this sends back XML / HTML or plain text depending on contentTypeStr
int send_xml_string( struct mg_connection *conn, const char* contentTypeStr, const std::string& s )
{
        mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %ld\r\n"        // Always set Content-Length
                "\r\n"
                "%s",
                contentTypeStr,
            s.length(), s.c_str());
        return 1;
}

int send_error_response( struct mg_connection *conn, const barzer::BarzerRequestParser& reqParser, const char* query, barzer::BarzerRequestParser::ErrInit err )
{
    const char* errStr = barzer::BarzerRequestParser::getErrInitText(err);
    std::stringstream sstr;
    switch(reqParser.ret) {
    case barzer::BarzerRequestParser::XML_TYPE:
        ay::XMLStream(sstr << "<error text=\"").escape(errStr) << "\"";
        ay::XMLStream(sstr << " query=\"").escape(query) << "\"/>";
        return send_xml_string( conn, reqParser.httpContentTypeString(), sstr.str() );
    case barzer::BarzerRequestParser::JSON_TYPE:
        ay::jsonEscape( query,
            ay::jsonEscape(errStr, sstr << "{ \"error\" : \"" ) << 
            "\", \"query\" : \""
        ) << "\" }";
        return send_json_string( conn, sstr.str() );
    default:
        sstr << "error:" << errStr << std::endl << 
        "query: \"" << query << "\"" << std::endl;
        return send_xml_string( conn, "text/plain; charset=utf-8", sstr.str() );
    }
}

static int begin_request_handler(struct mg_connection *conn) 
{
    const struct mg_request_info *request_info = mg_get_request_info(conn);
    bool invalidInput = ( !request_info || !request_info->uri || !request_info->query_string );

    const barzer::BarzerHttpServer& httpServ = barzer::BarzerHttpServer::instance();
    barzer::GlobalPools& gp = httpServ.gp; /// gp must not be changed - constant onsistency is hard to achieve but gp is const
    
    std::stringstream outSstr;

    barzer::BarzerRequestParser reqParser(gp,outSstr);
    barzer::BarzerRequestParser::ErrInit err = barzer::BarzerRequestParser::ERR_INIT_OK;

    std::string query;
    if( !invalidInput ) {
        std::string uri;
        ay::url_encode( uri, request_info->uri, strlen(request_info->uri) );
        ay::url_encode( query, request_info->query_string, strlen(request_info->query_string) );
        barzer::QuestionParm qparm;
        if( auto err = reqParser.initFromUri( qparm, uri.c_str(), uri.length(), query.c_str(), query.length() ) ) {
            return send_error_response( conn, reqParser, query.c_str(), err );
        } else 
            reqParser.parse(qparm);
    } else 
        return print_json_error(conn, "bad query string" );
    // Send HTTP reply to the client
    std::string contentStr = outSstr.str();

    if( contentStr.empty() )
        return send_error_response( conn, reqParser, query.c_str(), barzer::BarzerRequestParser::ERR_PROC_INTERNAL );

    mg_printf(conn,
        "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"        // Always set Content-Length
            "\r\n"
            "%s",
            reqParser.httpContentTypeString(),
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
