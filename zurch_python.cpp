#include <zurch_python.h>
#include <zurch_tokenizer.h>
#include <zurch_classifier.h>

#include <barzer_universe.h>
#include <barzer_shell.h>
#include <boost/python.hpp>
#include <barzer_python.h>
#include <ay/ay_cmdproc.h>
#include <ay_translit_ru.h>
#include <boost/python/list.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/object.hpp>
#include <boost/python/stl_iterator.hpp>

#include <barzer_parse.h>
#include <barzer_server_response.h>
#include <barzer_el_chain.h>
#include <barzer_emitter.h>
#include <boost/variant.hpp>

#include <util/pybarzer.h>

#include <autotester/barzer_at_comparators.h>
namespace zurch {

void pythonInit() 
{
    std::cerr << "pythonInit" << std::endl;
}

}
