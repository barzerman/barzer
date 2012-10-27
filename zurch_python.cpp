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

#include <barzer_bzspell.h>
#include <barzer_parse.h>
#include <barzer_server_response.h>
#include <barzer_el_chain.h>
#include <barzer_emitter.h>
#include <boost/variant.hpp>

#include <util/pybarzer.h>

#include <autotester/barzer_at_comparators.h>

using boost::python::stl_input_iterator ;
using namespace boost::python;
using namespace boost;

namespace zurch {
struct PythonClassifier {
    const ZurchPython& zp;
    PythonClassifier(const const ZurchPython& z): zp(z){}
};
struct PythonTokenizer {
    ZurchTokenizer tokenizer;
    const ZurchPython& zp;
    PythonTokenizer(const const ZurchPython& z): zp(z){}
    list tokenize( const std::string & s )
    {
        list retList;
        ZurchTokenVec tokVec;
        tokenizer.tokenize( tokVec, s.c_str(), s.length() );
        for( ZurchTokenVec::const_iterator i = tokVec.begin(); i!= tokVec.end(); ++i ) {
            retList.append(i->str);
        }
        return retList;
    }

};
struct PythonNormalizer {
    const ZurchPython& zp;
    ZurchTokenizer tokenizer;
    ZurchWordNormalizer normalizer;

    PythonNormalizer(const const ZurchPython& z): zp(z), 
    normalizer(zp.barzerPython->guaranteeUniverse().getBZSpell())
    {}
    list normalize( const std::string& s ) 
    {
        list retList;
        ZurchTokenVec tokVec;
        tokenizer.tokenize( tokVec, s.c_str(), s.length() );
        std::string norm;
        ZurchWordNormalizer::NormalizerEnvironment normEnv;
        for( ZurchTokenVec::const_iterator i = tokVec.begin(); i!= tokVec.end(); ++i ) {
            norm.clear();
            normalizer.normalize(norm,i->str.c_str(),normEnv);
            retList.append(norm);
        }
        return retList;
    }
};


PythonClassifier* ZurchPython::mkClassifier() const
{
    return new PythonClassifier(*this);
}
PythonTokenizer*  ZurchPython::mkTokenizer() const
{
    return new PythonTokenizer(*this);
}
PythonNormalizer* ZurchPython::mkNormalizer() const
{
    return new PythonNormalizer(*this);
}

ZurchPython::~ZurchPython() {delete barzerPython; }

ZurchPython::ZurchPython():
    barzerPython(new barzer::BarzerPython)
{ 
    barzerPython->guaranteeUniverse();
}

void ZurchPython::initInstance() 
{
    barzerPython->guaranteeUniverse();
}
void ZurchPython::init() 
{

    boost::python::class_<zurch::ZurchPython>( "Zurch" )
        .def( "mkClassifier", &ZurchPython::mkClassifier, return_value_policy<manage_new_object>() )
        .def( "mkTokenizer", &ZurchPython::mkTokenizer, return_value_policy<manage_new_object>() ) 
        .def( "mkNormalizer", &ZurchPython::mkNormalizer, return_value_policy<manage_new_object>() );

    boost::python::class_<zurch::PythonTokenizer>( "tokenizer", no_init )
        .def( "tokenize", &zurch::PythonTokenizer::tokenize );
    boost::python::class_<zurch::PythonNormalizer>( "normalizer", no_init )
        .def( "normalize", &zurch::PythonNormalizer::normalize );
    // std::cerr << "pythonInit" << std::endl;
}

} // namespace zurch
