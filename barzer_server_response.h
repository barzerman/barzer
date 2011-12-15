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
	BarzStreamerXML(const Barz &b, const StoredUniverse &u) : BarzResponseStreamer(b, u) {}
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
} // barzer namespace 
#endif /* BARZ_SERVER_RESPONSE_H_ */
