#pragma once

#include <vector>
#include <iostream>
/// iterates over a vector of states i-th state is a number 0-N , each state has a range of allowed values
namespace ay {

struct vsi_state {
    size_t curV=0, maxV=0;
    vsi_state( size_t m ) : curV(0), maxV(m) {}
    /// tries to increment
    bool operator++() { return( curV < maxV ? ++curV : false ); }
    vsi_state& reset() { return( curV=0, *this ); }
    operator size_t () const { return curV; }
    std::ostream& print( std::ostream& fp) const 
        { return fp << curV ; }
};

class vsi_state_vec {
    std::vector<vsi_state> d_vec;
    bool                   d_valid; 
public:
    typedef std::vector<vsi_state>::const_iterator const_iterator;
    const_iterator begin() const { return d_vec.begin(); }
    const_iterator end() const { return d_vec.end(); }

    vsi_state_vec() : d_valid(false){}
    const vsi_state& operator[]( size_t i ) const { return d_vec[i]; }
    size_t size() const { return d_vec.size(); }
    const std::vector<vsi_state>& state() const { return d_vec; }

    operator bool() const { return d_valid; }
    const vsi_state_vec& operator++() {
        if( !d_valid ) return *this;
        for( auto i = d_vec.begin(), last = d_vec.begin() + d_vec.size() -1; i != d_vec.end(); ++i ) {
            if( ++(*i) ) {
                break;
            } else if( i == last )  {
                d_valid = false;
                break;
            } else // carry 
                i->reset();
        }
        return *this;
    }
    vsi_state_vec& reset() 
    {
        for( auto& i: d_vec ) i.reset(); 
        d_valid = !d_vec.empty();
        return *this;
    }
    void addState( size_t m) 
    { 
        if(d_vec.empty()) 
            d_valid = true;
        d_vec.push_back( vsi_state(m) ); 
    }
    
    std::ostream& print( std::ostream& fp ) const 
        { for( const auto& i : d_vec ) i.print( fp ) << " "; return fp;}
    
    size_t state_capacity() const {
        size_t x = 1;
        for( const auto& i: d_vec ) 
            x*= (i.maxV+1);
        return x;
    }
};

} // namespace ay 
