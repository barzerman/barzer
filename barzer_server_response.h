/*
 * barzer_server_response.h
 *
 *  Created on: Apr 20, 2011
 *      Author: polter
 */

#ifndef BARZ_SERVER_RESPONSE_H_
#define BARZ_SERVER_RESPONSE_H_

#include <barzer_barz.h>
#include <barzer_universe.h>
#include <ay/ay_logger.h>
namespace  barzer {

class BarzResponseStreamer {
protected:
	const Barz &barz;
	StoredUniverse &universe;
public:
	BarzResponseStreamer(const Barz &b, StoredUniverse &u) : barz(b), universe(u) {}
	virtual std::ostream& print(std::ostream& os) { return os; }
	virtual ~BarzResponseStreamer() {}
};


class BarzStreamerXML : public BarzResponseStreamer {
public:
	BarzStreamerXML(const Barz &b, StoredUniverse &u) : BarzResponseStreamer(b, u) {}
	std::ostream& print(std::ostream&);
};

}

#endif /* BARZ_SERVER_RESPONSE_H_ */
