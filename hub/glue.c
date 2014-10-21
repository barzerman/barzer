#include "bh_expr.h"
#include "glue.h"

int test() {
    return 5;
}

Fun MkFun(char *s) {
    bh::FunTuple *ft = new bh::FunTuple();
    ft->compile(s);
    return (Fun)ft;
}

double apply(Fun fun, double* arr, size_t len) {
    std::vector<double> vec(arr, arr + len / sizeof(double));
    bh::Context ctx(vec);
    std::vector<double> ret;
    ((bh::FunTuple*)fun)->apply(ret, ctx);
    return ret[0];
}
