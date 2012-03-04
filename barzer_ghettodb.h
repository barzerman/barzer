#ifndef BARZER_GHETTODB_H 
#define BARZER_GHETTODB_H 

#include <ay/ay_string_pool.h>
#include <ay/ay_util_char.h>
#include <barzer_entity.h>

namespace barzer {
/// ghettodb is a very simple (entityid+key),value pair storage 
/// as of right now (march 2012) we thing the key will always be a single string 
/// for now the value is also a single string . the value type may actually change with time 
/// changes will always be backward compatible 
///  
class Ghettodb {
    ay::UniqueCharPool d_strPool; // all strings are pooled and interned 
public: 
    // this is uint32_t 
    typedef ay::UniqueCharPool::StrId StringId;

    typedef std::pair< StoredEntityUniqId, StringId >   Key;
    typedef StringId                                    Value; // for now value is always a string
    typedef std::map< Key, Value > KeyValueMap;
private:
    KeyValueMap d_kvMap; // main key value map
    
    template <typename T> void mkValue( Value&, const T& ) { std::cerr << "Ghettodb::mkValue source type not supported\n"; }

    void mkValue( Value& v, const ay::char_cp& x ) { v = d_strPool.internIt( x ); }
public: 
    
    void mkKey( Key& key, const StoredEntityUniqId& euid, const char* s ) 
    {
        key.first = euid;
        key.second = d_strPool.internIt(s);
    }
    // returns the second part of the key . passing in NULL should be safe (0xffffffff will be returned)
    StringId  getStringId( const char* s ) const { return d_strPool.getId(s); }
    
    const char* get_value_str_nullable( const Key& k ) const 
    {
        KeyValueMap::const_iterator i= d_kvMap.find( k );
        return( i == d_kvMap.end() ? 0 : d_strPool.resolveId(i->second) );
    }
    const char* get_value_str_safe( const Key& k ) {
        const char* s = get_value_str_nullable(k);
        if( s ) return s; else return "";
    }
    
    const char* get_value_str_nullable( const StoredEntityUniqId& euid, const char* s ) const 
        { return get_value_str_nullable( Key(euid, getStringId(s)) ); }

    const char* get_value_str_safe( const StoredEntityUniqId& euid, const char* k ) const 
        { const char* s =  get_value_str_nullable( Key(euid, getStringId(k)) ); if( s ) return s; else return ""; }
    
    template <typename T>
    KeyValueMap::const_iterator store( Key& k, const StoredEntityUniqId& euid, const char* keyStr, const T& val ) 
    {
        mkKey( k, euid, keyStr );
        Value v;
        mkValue( v, val );
        return d_kvMap.insert( KeyValueMap::value_type(k,v) ).first;
    }
    template <typename T>
    KeyValueMap::const_iterator store( const StoredEntityUniqId& euid, const char* keyStr, const T& val ) 
    {
        Key k;
        return store(k,euid,keyStr,val);
    }
};

}
#endif //  BARZER_GHETTODB_H 
