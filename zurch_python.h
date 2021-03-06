
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once

namespace barzer { class BarzerPython; };
namespace zurch {

struct PythonClassifier;
struct PythonTokenizer;
struct PythonNormalizer;

struct ZurchPython {
    barzer::BarzerPython* barzerPython;

    PythonClassifier* mkClassifier();
    PythonTokenizer*  mkTokenizer() const;
    PythonNormalizer* mkNormalizer() const;

    void initInstance();

    static void init();
    ZurchPython();
    ~ZurchPython();
}; 

} /// namespace zurch
