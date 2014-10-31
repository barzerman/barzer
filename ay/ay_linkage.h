#pragma once

#include <boost/unordered_map.hpp>
#include <vector>

namespace ay {

typedef std::pair<uint32_t, float> wil_link_data;

inline bool operator<(const wil_link_data& l, const wil_link_data& r) { return l.first < r.first; }

class weighed_id_linkage {
public:
    typedef std::vector<wil_link_data> link_data_vec;
private:
    boost::unordered_map<uint32_t, link_data_vec> d_map;
public:
    typedef decltype(d_map)::value_type value_type;
    link_data_vec& produce_vec(uint32_t id) 
        { return d_map.insert({id, link_data_vec()}).first->second; }

    const link_data_vec* get_vec(uint32_t id) const
    {
        auto i = d_map.find(id);
        return(i==d_map.end()? 0: &(i->second));
    }
    link_data_vec& sorted_vec(uint32_t id)
    {
        auto& v = produce_vec(id);
        std::sort(v.begin(), v.end());
        return v;
    }
    // override - when 0 value wont be overridden, when -1 - override happens f new value is smaller, for 1 - when greater 
    bool insert_sorted(uint32_t id, const wil_link_data& ld, int override_val=0)
    {
        auto& v = produce_vec(id);
        auto i = std::lower_bound(v.begin(), v.end(), ld);

        if(i == v.end())
            return( v.push_back(ld), true );
        else if(i->first == ld.first)
            return( override_val? (i->second = ld.second, true): false );
        else
            return( v.insert(i, ld), true );
    }
    void sort_all() { for(auto& v: d_map) std::sort(v.second.begin(), v.second.end()); }
    
    template <typename CB> void iterate(const CB& cb) { for(auto& v: d_map) cb(v); }
    template <typename CB> void iterate(CB& cb) { for(auto& v: d_map) cb(v); }
};

}
