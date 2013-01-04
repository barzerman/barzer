#pragma once
#include <ay_bitflags.h>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
namespace ay {

struct  dir_iter_t {
public: 
    enum { 
        BIT_DIR_RECURSE,
        BIT_FOLLOW_LINKS,
        BIT_MAX
    };
    ay::bitflags<BIT_MAX> d_bits;
    
    template <typename CB>
    void operator()( CB& cb, boost::filesystem::path path, size_t depth=0 ) 
    {
        if ( boost::filesystem::exists(path) && boost::filesystem::is_directory(path)) {
            boost::filesystem::directory_iterator end_iter;
            for( boost::filesystem::directory_iterator di(path) ; di != end_iter ; ++di) {
                if( !cb(di,depth) ) 
                    return;
                if (boost::filesystem::is_regular_file(di->status()) ) {
                    ;
                } else if (boost::filesystem::is_directory(di->status()) ) {
                    dir_iter_t nextLevel(*this);
                    if( d_bits.check(BIT_DIR_RECURSE) )
                        nextLevel(cb,di->path(),depth+1);
                } 
            }
        }
    }
};

struct dir_iter_print_t {
    std::ostream& fp;
    dir_iter_print_t( std::ostream& f ) : fp(f) {}

    /// returns false if wants recursion terminated 
    bool operator()( boost::filesystem::directory_iterator& di, size_t depth ) {
        for( size_t i=0; i< depth; ++i ) fp << '\t';

        fp << di->path().parent_path().string() << '/' << di->path().filename().string() << ( boost::filesystem::is_directory(di->status()) ? "/" : ""  ) << std::endl; 
        return true;
    }
};
/// this can be used to add regexp filter on filename (only the filename will be searched for the regexp)
template <typename CB>
struct fs_filter_regex {
public:
    CB&              callback;
    boost::regex    pattern;
    
    fs_filter_regex( CB& cb ): callback(cb) {}
    fs_filter_regex( CB& cb, const char* p ) : callback(cb), pattern(p) {}

    bool operator()( boost::filesystem::directory_iterator& di, size_t depth=0 ) {
        if( pattern.empty() || boost::regex_search (di->path().filename().string(), pattern) )
            return callback(di,depth); 
        return true;
    }
};

/// this function is here mainly 
inline void dir_list( std::ostream& fp, const char* path, bool recurse=false, const char* regex=0 )
{
    dir_iter_print_t printer(fp);
    fs_filter_regex<dir_iter_print_t> fs_regex_iter(printer);

    dir_iter_t dip;
    if( recurse ) 
        dip.d_bits.set( dir_iter_t::BIT_DIR_RECURSE );
    if( regex ) 
        fs_regex_iter.pattern= regex;

    dip( fs_regex_iter, std::string(path) );
}

} // namespace ay
