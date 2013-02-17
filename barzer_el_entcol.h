
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once

#include <ay/ay_pool_with_id.h>

namespace barzer {

/// a single group of entities 
/// later on we can add rules for adding/ordering entities to the group
/// we may decide to keep up to N entities by relevance in any group, etc.
struct EntityGroup {
	typedef std::vector< uint32_t > Vec;
	Vec d_vec;
	const Vec& getVec() const { return d_vec; }
	void addEntity( uint32_t i ) { 
		d_vec.push_back( i ); 
	} 
};
//// pool of entity groups
class EntityCollection {
public:
	ay::PoolWithId<EntityGroup> d_entVecPool;
public:
	EntityGroup* addEntGroup( uint32_t& id ) 
		{ return d_entVecPool.addObj( id ); }
	const EntityGroup* getEntGroup( uint32_t id ) const { return d_entVecPool.getObjById(id); }
	EntityGroup* getEntGroup( uint32_t id ) { return d_entVecPool.getObjById(id); }
};

} // namespace barzer 
