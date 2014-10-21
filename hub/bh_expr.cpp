#include "bh_expr.h"
#include <unordered_map>
#include <limits>

using boost::property_tree::ptree;
namespace bh {

typedef std::unordered_map<std::string, Fun> FunIndex;

#define FUN_REC(n) #n, [](params& ret, const params& args, const Context& c)
FunIndex& get_fun_ix() {
    static FunIndex fun_ix = {
        { FUN_REC(values) { // retrieves input values from the context
            for (const double& a: c.values)
                ret.push_back(a);
        }},
        { FUN_REC(add) {
            double val = 0.0;
            for (const double& a : args) {
                val += a;
            }
            ret.push_back(val);
        }},
        { FUN_REC(sub) {
            double val = 0.0;
            if (args.size() > 0) {
                auto it = args.begin();
                val = *it++;
                for (; it != args.end(); ++it) {
                    val -= *it;
                }
            }
            ret.push_back(val);
        }},
        { FUN_REC(mul) {
            // change next line to val = 1 if you want the product of 
            // an empty list to be 1
            double val = std::min(1.0, (double)args.size()); // either 0 or 1
            for (const double& a : args) {
                val *= a;
            }
            ret.push_back(val);
        }},
        { FUN_REC(div) {
            double val = 0.0;
            if (args.size() > 0) {
                auto it = args.begin();
                val = *it++;
                for (; it != args.end(); ++it) {
                    if (*it == 0.0) {
                        std::cerr << "Divison by zero detected; skipping\n";
                        continue;
                    }
                    val /= *it;
                }
            }
            ret.push_back(val);
        }},
        { FUN_REC(avg) {
            double val = 0.0;
            if (args.size() == 0) return;
            for (const double& a : args) {
                val += a;
            }
            val /= args.size();
            ret.push_back(val);
        }},
        // the next two default to +-infinity
        { FUN_REC(max) {
            double val = -std::numeric_limits<double>::infinity();
            for (const double& a : args) {
                if (a > val) val = a;
            }
            ret.push_back(val);
        }},
        { FUN_REC(min) {
            double val = std::numeric_limits<double>::infinity();
            for (const double& a : args) {
                if (a < val) val = a;
            }
            ret.push_back(val);
        }},

    };
    return fun_ix;
}
#undef FUN_REC

struct Value { // value (or rather constant) function 
    double val;
    Value(double v) : val(v) {}
    void operator() (params& ret, const params& args, const Context ctx) {
        ret.push_back(val);
    }
};

void FunTuple::compile(const ptree::value_type &v) {
    FunIndex& fun_ix = get_fun_ix();
    if (v.first == "val") { // special case: constant value
        const boost::optional<double> of = v.second.get_optional<double>("<xmlattr>.of");
        if (of) {
            d_fun = Value(of.get());
        } else {
            std::cerr << "Wrong value (expected <val of='N' />)" << std::endl;
        }
    } else { // normal functions
        FunIndex::const_iterator fi = fun_ix.find(v.first);
        if (fi != fun_ix.end()) {
            d_fun = fi->second;
            d_children.resize(v.second.size());
            auto cit = d_children.begin();
            for (const ptree::value_type &c : v.second) {
                (cit++)->compile(c);
            }
        } else {
            std::cerr << "Unknown function: " << v.first << std::endl;
        }
    }
}


void FunTuple::compile(const ptree& pt) {
    compile(pt.front());
}

void FunTuple::compile(const char * str) {
    std::stringstream ss(str);
    ptree pt;
    read_xml(ss, pt);
    compile((const ptree)pt);
}

void FunTuple::apply(params& ret, const Context& ctx) {
    params args;
    args.reserve(d_children.size());
    for (FunTuple &c : d_children) {
        c.apply(args, ctx);
    }
    d_fun(ret, args, ctx);
}


}

