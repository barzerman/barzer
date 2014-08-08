
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * barzer_server_response.h
 *
 *  Created on: Apr 20, 2011
 *      Author: polter
 */
#pragma once
#include <barzer_barz.h>
#include <ay/ay_logger.h>
#include <ay/ay_bitflags.h>

namespace  barzer {
class StoredUniverse;

class BarzResponseStreamer {
protected:
	const Barz &barz;
	const StoredUniverse &universe;
    const QuestionParm& qparm;
public:
    enum { 
        BF_NOTRACE, // by default trace is printed
        BF_USERID,
        BF_ORIGQUERY,
        BF_QUERYID,
        BF_NO_ORIGOFFSETS, // when set no original markup is returned inside the beads 
        BF_SIMPLIFIED, // when set simplified mode is assumed
        /// add new flags above this line
        BF_MAX
    };
    typedef ay::bitflags<BF_MAX> ModeFlags;
    ModeFlags d_outputMode; 

    void setSimplified( bool x= true ) 
        { d_outputMode.set( BF_SIMPLIFIED,x ); }
    bool isSimplified() const { return d_outputMode.check( BF_SIMPLIFIED ); }

    void setComparatorMode(const ModeFlags& m ) { d_outputMode= m; }
    void setComparatorMode() { 
        d_outputMode.set( BF_NOTRACE );
        d_outputMode.set( BF_USERID );
        d_outputMode.set( BF_ORIGQUERY );
        d_outputMode.set( BF_QUERYID );
        d_outputMode.set( BF_NOTRACE );
        d_outputMode.set( BF_NO_ORIGOFFSETS );
    }
    void setBit( int i , bool val = true ) { d_outputMode.set(i,val); }
    bool checkBit( int i ) const { return d_outputMode.checkBit( i ); }
    void setWholeMode( const ModeFlags& m ) { d_outputMode= m; }

	BarzResponseStreamer(const Barz &b, const StoredUniverse &u, const QuestionParm& qp ) : barz(b), universe(u), qparm(qp) {}
	BarzResponseStreamer(const Barz &b, const StoredUniverse &u, const ModeFlags& mf, const QuestionParm& qp) : 
        barz(b), universe(u), qparm(qp), d_outputMode(mf) 
    {}
	virtual std::ostream& print(std::ostream& os) { return os; }
	virtual ~BarzResponseStreamer() {}
};


class BarzStreamerXML : public BarzResponseStreamer {
public:
	BarzStreamerXML(const Barz &b, const StoredUniverse &u, const QuestionParm& qp) : BarzResponseStreamer(b, u, qp) {}
	BarzStreamerXML(const Barz &b, const StoredUniverse &u, const ModeFlags& mf, const QuestionParm& qp) : 
        BarzResponseStreamer(b, u, mf, qp)
    {}

	std::ostream& printConfidence(std::ostream&);
	std::ostream& print(std::ostream&);
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
