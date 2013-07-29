
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once

#include <ay/ay_util.h>
#include <ay/ay_xml_util.h>
#include <barzer_server_response.h>

namespace barzer {

using ay::json_raii ;
class StoredUniverse;

class BarzStreamerJSON : public BarzResponseStreamer {
public:
	BarzStreamerJSON(const Barz &b, const StoredUniverse &u) : BarzResponseStreamer(b, u) {}
	BarzStreamerJSON(const Barz &b, const StoredUniverse &u, const ModeFlags& mf) : 
        BarzResponseStreamer(b, u, mf)
    {}

	std::ostream& print(std::ostream&);
    static std::ostream& print_entity_fields(std::ostream& os, const BarzerEntity &euid, const StoredUniverse& universe );

};


} // namespace barzer 
