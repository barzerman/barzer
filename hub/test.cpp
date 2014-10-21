#include "bh_expr.h"
//*

int main() {
    const char* s = "<add><max><values /></max><avg><values /></avg><val of='6' /></add>";
    //const char* s = "<sub><val of='10' /><val of='5' /><val of='3' /></sub>";
    //const char* s = "<div><val of='10' /><val of='5' /><val of='0' /></div>";
    //const char* s = "<min></min>";
    //const char* s = "<max></max>";
    bh::FunTuple ft;
    ft.compile(s);
    bh::params args = {1.0, 0.9, 0.7};
    bh::Context ctx(args);
    bh::params v;
    ft.apply(v, ctx);
    std::cout << v[0] << std::endl;
}//*/