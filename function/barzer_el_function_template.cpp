#include <barzer_el_function.h>
#include <barzer_el_function_holder.h>

namespace barzer {
using namespace funcHolder;
namespace {
/// FUNCTIONS HERE

BELFunctionStorage_holder::DeclInfo g_funcs[] = {
/// FUNCTION INITIALIZATIONS HERE
};

} // anonymous namespace

namespace funcHolder {

/// XXX - replace with the name of this function type
void loadAllFunc_XXX(BELFunctionStorage_holder* holder)
    { for( const auto& i : g_funcs ) holder->addFun( i ); }
}

} // namespace barzer
