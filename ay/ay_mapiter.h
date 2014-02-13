#pragma once

namespace ay {

template <typename T>
struct mapiter_class {
    T& m;
    typename T::iterator i;

    mapiter_class( T& x, const typename T::key_type& k ) : m(x), i(x.find(k)) {}
    typename T::iterator operator ->( ) const { return i; }
    typename T::value_type& operator *( ) const { return *i; }
    operator bool() const { return i != m.end(); }
};
template <typename T> mapiter_class<T> mapiter( T& x, const typename T::key_type& k ) { return { x, k }; }

}
