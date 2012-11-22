#pragma once 

#include <sstream>
#include <boost/python.hpp>

namespace ay {

namespace python {

struct object_extract {

    template <typename T>
    bool extract( T& t, const boost::python::object& o )
    {
        boost::python::extract<T> e(o);
        if ( e.check() ) 
            return ( t=e, true );
        else
            return false;
    }


    template <typename T>
    bool operator()( T& t, const boost::python::object& o)
        { return extract(t,o); }
    bool operator()( std::string& t, const boost::python::object& o)
    {
        if( extract(t,o) ) 
            return true;
        /// trying to extract 
        { /// int
        int x = 0.0;
        if( extract(x,o) ) {
            std::stringstream sstr;
            sstr<< x;
            return ( t=sstr.str(),true );
        }
        } // end of int 
        { /// double
        double x = 0.0;
        if( extract(x,o) ) {
            std::stringstream sstr;
            sstr<< x;
            return ( t=sstr.str(),true );
        }
        } // end of double
        return false;
    }
};

} // namesapce python

} // namespace ay 
