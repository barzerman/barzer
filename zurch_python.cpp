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
struct PythonTokenizer {
    ZurchTokenizer tokenizer;
    const ZurchPython& zp;
    PythonTokenizer(const ZurchPython& z): zp(z){}
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

    PythonNormalizer(const ZurchPython& z): zp(z), 
    normalizer(zp.barzerPython->guaranteeUniverse().getBZSpell())
    {}
    /// must delete the returned pointer
    char* internal_tokenize( ZurchTokenVec& tokVec, const std::string& sorig )
    {
        size_t lc_sz = sorig.length()+1;
        int lang = barzer::Lang::getLangNoUniverse( sorig.c_str(), sorig.length() );

        char * lc= new char[ lc_sz ];

        memcpy( lc, sorig.c_str(), sorig.length() );
        lc[ lc_sz-1 ] =0;
        barzer::Lang::stringToLower( lc, lc_sz, lang );

        tokenizer.tokenize( tokVec, lc, lc_sz );
        return lc;
    }

    list tokenize( const std::string& s ) 
    {
        list retList; 
        ZurchTokenVec tokVec;
        tokenizer.tokenize( tokVec, s.c_str(), s.length() );
        for( ZurchTokenVec::const_iterator i = tokVec.begin(); i!= tokVec.end(); ++i ) {
            retList.append(i->str);
        }
        return retList;
    }
    void internal_normalize( std::vector< std::string > & vec, const std::string& s ) 
    {
        ZurchTokenVec tokVec;
        /// this is needed so that the char* inside tokVec are valid
        std::auto_ptr<char>( internal_tokenize( tokVec, s ) );
        std::string norm;
        ZurchWordNormalizer::NormalizerEnvironment normEnv;
        for( ZurchTokenVec::const_iterator i = tokVec.begin(); i!= tokVec.end(); ++i ) {
            norm.clear();
            if( i->str.length() > 3 ) {
                normalizer.normalize(norm,i->str.c_str(),normEnv);
                vec.push_back( norm );
            } else 
                vec.push_back(i->str);
        }
    }
    list normalize( const std::string& sorig ) 
    {
        list retList;
        ZurchTokenVec tokVec;
        /// this is needed so that the char* inside tokVec are valid
        std::auto_ptr<char>( internal_tokenize( tokVec, sorig ) );
        std::string norm;
        ZurchWordNormalizer::NormalizerEnvironment normEnv;
        for( ZurchTokenVec::const_iterator i = tokVec.begin(); i!= tokVec.end(); ++i ) {
            norm.clear();
            if( i->str.length() > 3 ) {
                normalizer.normalize(norm,i->str.c_str(),normEnv);
                retList.append(norm);
            } else 
                retList.append(i->str);

        }
        return retList;
    }
};


struct PythonClassifier {
    ZurchPython& zp;
    PythonNormalizer normalizer; 
    PythonClassifier(ZurchPython& z): zp(z), normalizer(zp){}

    list tokenize( const std::string& s ) 
        { return normalizer.tokenize(s); }
    list normalize( const std::string& s ) 
        { return normalizer.normalize(s); }
    void classify( const list& l )
    {
        for( size_t i = 0, i_end = len(l); i< i_end; ++i ) {
            std::string s = boost::python::extract<std::string>(l[i]);
            std::cerr << s << std::endl;
        }
    }
};
PythonClassifier* ZurchPython::mkClassifier() 
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
    boost::python::class_<zurch::PythonClassifier>( "classifier", no_init )
        .def( "tokenize", &zurch::PythonClassifier::tokenize )
        .def( "normalize", &zurch::PythonClassifier::normalize )
        .def( "classify", &zurch::PythonClassifier::classify );
    // std::cerr << "pythonInit" << std::endl;
}

} // namespace zurch
