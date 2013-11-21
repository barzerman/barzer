#pragma once 

#include <vector>
#include <string>

namespace barzer {
namespace barzerlib {


class Instance {
    void* d_data;
public:
    Instance();
    ~Instance();

    Instance& init( const std::vector< std::string >& argv );
    Instance& run_shell();
    Instance& run_server(uint16_t port);

    enum {
        URIERR_ERR_OK,
        URIERR_BAD_URI
    };
    int processUri( std::ostream& , const char* uri );
};

} // namespace barzerlib

} // namespace barzer 
