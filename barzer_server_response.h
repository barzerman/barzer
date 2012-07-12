/*
 * barzer_server_response.h
 *
 *  Created on: Apr 20, 2011
 *      Author: polter
 */

#ifndef BARZ_SERVER_RESPONSE_H_
#define BARZ_SERVER_RESPONSE_H_

#include <barzer_barz.h>
#include <ay/ay_logger.h>
#include <ay/ay_bitflags.h>

namespace  barzer {
class StoredUniverse;

class BarzResponseStreamer {
protected:
	const Barz &barz;
	const StoredUniverse &universe;
public:
	BarzResponseStreamer(const Barz &b, const StoredUniverse &u) : barz(b), universe(u) {}
	virtual std::ostream& print(std::ostream& os) { return os; }
	virtual ~BarzResponseStreamer() {}
};


class BarzStreamerXML : public BarzResponseStreamer {
public:
    enum { 
        BF_NOTRACE, // by default trace is printed
        BF_USERID,
        BF_ORIGQUERY,
        BF_QUERYID,
        /// add new flags above this line
        BF_MAX
    };
    typedef ay::bitflags<BF_MAX> ModeFlags;
    ModeFlags d_outputMode; 
    void setComparatorMode() { 
        d_outputMode.set( BF_NOTRACE );
        d_outputMode.set( BF_USERID );
        d_outputMode.set( BF_ORIGQUERY );
        d_outputMode.set( BF_QUERYID );
        d_outputMode.set( BF_NOTRACE );
    }

    bool checkBit( int i ) const { return d_outputMode.checkBit( i ); }

	BarzStreamerXML(const Barz &b, const StoredUniverse &u) : BarzResponseStreamer(b, u) {}
	std::ostream& print(std::ostream&);
    void setWholeMode( const ModeFlags& m ) { d_outputMode= m; }

};

class BestEntities;

class AutocStreamerJSON {
    const BestEntities& bestEnt;
    const StoredUniverse &universe;
    
public:
	AutocStreamerJSON(const BestEntities&b, const StoredUniverse &u) : 
        bestEnt(b), universe(u) 
    {}
	std::ostream& print(std::ostream&) const;

};

std::ostream& xmlEscape(const char *src,  std::ostream &os) ;


} // barzer namespace 
#endif /* BARZ_SERVER_RESPONSE_H_ */
