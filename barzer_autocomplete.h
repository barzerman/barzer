#ifndef BARZER_AUTOCOMPLETE_H
#define BARZER_AUTOCOMPLETE_H
#include <iostream>
namespace barzer {

class StoredUniverse;
class Barz;
struct QuestionParm;

class BarzerAutocomplete {
    Barz& d_barz;
    const StoredUniverse& d_universe;
    QuestionParm& d_qparm;
    std::ostream& d_os;
public:
    BarzerAutocomplete( Barz& barz, const StoredUniverse& universe, QuestionParm& qparm, std::ostream& os ) : 
        d_barz(barz), d_universe(universe), d_qparm(qparm), d_os(os) {}
    
    int parse( const char* q );
};

}
#endif // BARZER_AUTOCOMPLETE_H
