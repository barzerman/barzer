#pragma once 
#include <barzer_basic_types.h>
#include <boost/variant/recursive_variant.hpp>
#include <boost/optional/optional.hpp>
#include <boost/foreach.hpp>


#include <barzer_parse_types.h>
namespace barzer {

/// range of continuous values (int,real,date,time ...)
struct BarzerRange {
	typedef std::pair< BarzerNone, BarzerNone > None;
	typedef std::pair< int64_t, int64_t > Integer;
	typedef std::pair< double, double > Real;
	typedef std::pair< BarzerTimeOfDay, BarzerTimeOfDay > TimeOfDay;
	typedef std::pair< BarzerDate, BarzerDate > Date;
	typedef std::pair< BarzerDateTime, BarzerDateTime > DateTime;
	typedef std::pair< BarzerEntity, BarzerEntity > Entity;
	typedef std::pair< BarzerLiteral, BarzerLiteral > Literal;

	typedef boost::variant<
		None,
		Integer,
		Real,
		TimeOfDay,
		Date,
		DateTime,
		Entity,
        Literal
	> Data;

	enum {
		None_TYPE,
		Integer_TYPE,
		Real_TYPE,
		TimeOfDay_TYPE,
		Date_TYPE,
		DateTime_TYPE,
		Entity_TYPE,
		Literal_TYPE
	};

	Data dta;

	enum {
		ORDER_ASC,
		ORDER_DESC
	};
    const char* jsonGetTypeName() const {
        switch( dta.which() ) {
		case None_TYPE: return "none";
		case Integer_TYPE: return "integer";
		case Real_TYPE: return "real";
		case TimeOfDay_TYPE: return "time";
		case Date_TYPE:  return "date";
		case DateTime_TYPE: return "timestamp";
		case Entity_TYPE: return "entity";
		case Literal_TYPE: return "literal";
        default: return "none";
        }
    }
	bool isNone( ) const {return (dta.which() ==None_TYPE); }
	bool isValid( ) const {return dta.which(); }
	bool isInteger( ) const {return (dta.which() ==Integer_TYPE); }
	bool isReal( ) const {return (dta.which() ==Real_TYPE); }
    bool isNumeric() const { return (isInteger() || isReal()); }
	bool isTimeOfDay( ) const {return (dta.which() ==TimeOfDay_TYPE); }
	bool isDate( ) const {return (dta.which() ==Date_TYPE); }
	bool isDateTime( ) const {return (dta.which() ==DateTime_TYPE); }
	bool isEntity( ) const {return (dta.which() ==Entity_TYPE); }
	bool isLiteral( ) const {return (dta.which() ==Literal_TYPE); }

    const BarzerRange& promote_toReal( );

	const None* getNone( ) const { return boost::get<None>( &dta ); }
	const Integer*  getInteger( ) const {return boost::get<Integer>( &dta ); }
	const Real*  getReal( ) const {return boost::get<Real>( &dta ); }
	const TimeOfDay*  getTimeOfDay( ) const {return boost::get<TimeOfDay>( &dta ); }
	const Date* getDate( ) const {return boost::get<Date>( &dta ); }
	const DateTime* getDateTime( ) const {return boost::get<DateTime>( &dta ); }
	const Entity* getEntity( ) const {return boost::get<Entity>( &dta ); }
	const Literal* getLiteral( ) const {return boost::get<Literal>( &dta ); }

	uint8_t order;  // ASC/DESC
	enum {
		RNG_MODE_FULL, // low to high
		RNG_MODE_NO_HI, // above low (no hi limit)
		RNG_MODE_NO_LO // below hi (no low limit)
	};
	uint8_t rng_mode;  // one of the RNG_MODE_XXX constants
	int getRngMode() const { return rng_mode; }

	std::ostream& print( std::ostream& fp ) const;
	bool hasLo() const { return ( rng_mode == RNG_MODE_FULL || rng_mode == RNG_MODE_NO_HI ); }
	bool hasHi() const { return ( rng_mode == RNG_MODE_FULL || rng_mode == RNG_MODE_NO_LO ); }

	bool isFull() const { return ( rng_mode == RNG_MODE_FULL ); }
	bool isNoLo() const { return (rng_mode == RNG_MODE_NO_LO); }
	bool isNoHi() const { return (rng_mode == RNG_MODE_NO_HI); }

	void setNoHI() { rng_mode= RNG_MODE_NO_HI; }
	void setNoLO() { rng_mode= RNG_MODE_NO_LO; }
	void setFullRange() { rng_mode= RNG_MODE_FULL; }

	
        enum {
            RANGE_MOD_SCALE_MULT,
            RANGE_MOD_SCALE_DIV,
            RANGE_MOD_SCALE_PLUS,
            RANGE_MOD_SCALE_MINUS         
        } ;
    /// scales the range if its numeric . returns false if failed to scale (incompatible types etc)
    bool scale( const BarzerNumber& n, int mode  );
    
    bool scale( const BarzerNumber& n1,const BarzerNumber& n2, int mode) ;
	char geXMLtAttrValueChar( ) const ;

	void setData(const Data &d) { dta = d; }
	const Data& getData() const { return dta; }
    
    template <typename T>  T* get() { return boost::get<T>(&dta); }
    template <typename T>  T* set() { return (dta = T(), boost::get<T>(&dta)); }

	Data& getData() { return dta; }
	uint32_t getType() const { return dta.which(); }

	bool isBlank() const { return !dta.which(); }
	void setAsc() { order = ORDER_ASC; }
	void setDesc() { order = ORDER_DESC; }
	bool isAsc() const { return ORDER_ASC== order; }
	bool isDesc() const { return ORDER_DESC== order; }

	BarzerRange( ) : order(ORDER_ASC), rng_mode(RNG_MODE_FULL) {}

	struct Empty_visitor : public boost::static_visitor<bool> {
        template <typename T> bool operator()( const T& val ) const { return (val.first == val.second); }
        bool operator() ( const BarzerRange::Real& val ) const
            { return ( fabs(val.first - val.second) <  0.0000000000001); }
    };
    bool isEmpty() const
        { return (rng_mode == RNG_MODE_FULL && boost::apply_visitor( Empty_visitor(), dta )); }

	struct Less_visitor : public boost::static_visitor<bool> {
		// typedef BarzerRange::Data Data;
		const BarzerRange& leftVal;
		Less_visitor( const BarzerRange& r ) : leftVal(r) {}

		template <typename T> bool operator()( const T& rVal ) const {
			// here we're guaranteed that rng_modes are the same
			switch( leftVal.rng_mode ) {
			case BarzerRange::RNG_MODE_FULL: return (boost::get<T>( leftVal.dta ) < rVal);
			case BarzerRange::RNG_MODE_NO_HI: return (boost::get<T>( leftVal.dta ).first < rVal.first);
			case BarzerRange::RNG_MODE_NO_LO: return (boost::get<T>( leftVal.dta ).second < rVal.second );
			}
			return false;
		}
	};
	struct Equal_visitor : public boost::static_visitor<bool> {
		typedef BarzerRange::Data Data;
		const BarzerRange& leftVal;
		Equal_visitor( const BarzerRange& l ) : leftVal(l) {}
		template <typename T> bool operator()( const T& rVal ) const {
			switch( leftVal.rng_mode ) {
			case BarzerRange::RNG_MODE_FULL: return (boost::get<T>( leftVal.dta ) == rVal);
			case BarzerRange::RNG_MODE_NO_HI: return (boost::get<T>( leftVal.dta ).first == rVal.first);
			case BarzerRange::RNG_MODE_NO_LO: return (boost::get<T>( leftVal.dta ).second == rVal.second );
			}
			return true;
		}
	};

	bool lessThan( const BarzerRange& r ) const
	{
		if( dta.which() < r.dta.which() )
			return true;
		else if( r.dta.which() < dta.which() )
			return false;
		else { /// this guarantees that the left and right types are the same
			if( getRngMode() < r.getRngMode() )
				return true;
			else if( r.getRngMode()  < getRngMode() )
				return false;
			else
				return boost::apply_visitor( Less_visitor(*this), r.dta );
		}
	}
	bool isEqual( const BarzerRange& r )  const
	{
		if( dta.which() != r.dta.which() )
			return false;
		else  /// this guarantees that the left and right types are the same
			return ( getRngMode() == r.getRngMode()  && boost::apply_visitor( Equal_visitor(*this), r.dta ) );
	}
	Entity& setEntityClass( const StoredEntityClass& c ) { return boost::get<Entity>(dta = Entity( StoredEntityUniqId(c), StoredEntityUniqId(c))); }
	Entity& setEntity( ) { return( boost::get<Entity>(dta = Entity())); }

	Entity& setEntity( const StoredEntityUniqId& euid1, const StoredEntityUniqId& euid2 ) { return boost::get<Entity>( dta = Entity(euid1,euid2)); }

	Literal& setLiteral( const BarzerLiteral& ltrl1, const BarzerLiteral& ltrl2 ) { return boost::get<Literal>( dta = Literal(ltrl1,ltrl2)); }
};
inline bool operator== ( const BarzerRange& l, const BarzerRange& r ) { return l.isEqual(r); }
inline bool operator< ( const BarzerRange& l, const BarzerRange& r ) { return l.lessThan(r); }
inline std::ostream& operator <<( std::ostream& fp, const BarzerRange& x )
	{ return( x.print(fp) ); }

/// combination


struct BarzerERC {
	BarzerEntity d_entId; // main entity id
	BarzerEntity d_unitEntId; // unit entity id
	BarzerRange  d_range;
    
    BarzerERC() {}
    explicit BarzerERC( const BarzerEntity& e ) : d_entId(e) {}

	const BarzerEntity& getEntity() const { return d_entId; }
	const BarzerEntity& getUnitEntity() const { return d_unitEntId; }
	const BarzerRange&  getRange() const { return d_range; }
	BarzerEntity& getEntity() { return d_entId; }
	BarzerEntity& getUnitEntity() { return d_unitEntId; }
	BarzerRange&  getRange() { return d_range; }

	BarzerRange*  getRangePtr() { return &d_range; }
	const BarzerRange*  getRangePtr() const { return &d_range; }

    int64_t getInt( int64_t dfVal ) const
    {
        if( d_range.hasLo() ) {
            const BarzerRange::Integer* rng = d_range.getInteger();
            if( rng ) {
                return rng->first;
            }
        }
        return dfVal;
    }

	void  setEntity( const BarzerEntity& e ) { d_entId = e; }
	void  setUnitEntity( const BarzerEntity& e ) { d_unitEntId = e; }
	void  setRange( const BarzerRange& r ) { d_range = r; }

	std::ostream& print( std::ostream& fp ) const
		{ return ( d_range.print( fp )<<"("  << d_entId << ":" << d_unitEntId << ")" ); }
	bool isEqual( const BarzerERC& r ) const
	{ return( (d_entId == r.d_entId) && (d_unitEntId == r.d_unitEntId) && (d_range == r.d_range) ); }
	bool lessThan( const BarzerERC& r ) const
	{
		return ay::range_comp( ).less_than(
			d_entId, d_unitEntId, d_range,
			r.d_entId, r.d_unitEntId, r.d_range
		);
	}
	// only range type is checked if at all
	bool matchOther( const BarzerERC& other, bool checkRange=false ) const {
		return (
			d_entId.matchOther( other.d_entId )  &&
			d_unitEntId.matchOther( other.d_unitEntId ) &&
			(!checkRange || d_range.getType() == other.getRange().getType() )
		);
	}

	bool matchOtherWithBlanks(
		const BarzerERC& other, bool matchBlankRange, bool matchBlankEnt
	) const {
		return (
			d_entId.matchOther( other.d_entId, matchBlankEnt )  &&
			d_unitEntId.matchOther( other.d_unitEntId, matchBlankEnt ) &&
			(d_range.getType() == other.getRange().getType() || (!matchBlankRange && !d_range.isValid() ))
		);
	}
    const BarzerEntity& mainEntity() const { return d_entId; }
    const BarzerEntity& unitEntity() const { return d_unitEntId; }
    const BarzerRange& range() const { return d_range; }
};

inline bool operator ==( const BarzerERC& l, const BarzerERC& r ) { return l.isEqual(r); }

inline bool operator <( const BarzerERC& l, const BarzerERC& r ) { return l.lessThan(r); }
inline std::ostream& operator <<( std::ostream& fp, const BarzerERC& l ) { return l.print(fp); }

/// ERC expression for example (ERC and ERC ... )
struct BarzerERCExpr {
	typedef boost::variant<
		BarzerERC,
		boost::recursive_wrapper<BarzerERCExpr>
	> Data;

	typedef std::vector< Data > DataList;
	DataList d_data;

	enum {
		T_LOGIC_AND,
		T_LOGIC_OR,
		// add hardcoded expression types above this line
		T_LOGIC_CUSTOM
	};
	uint16_t d_type; // one of T_XXX values
    // EC stands for Expression Type
	enum {
		EC_LOGIC, /// default - logical expressions 
		EC_ARBITRARY=0xffff
	};
	uint16_t d_eclass; // for custom types only currently unused. default 0

	BarzerERCExpr( const BarzerERCExpr& x ) : d_data(x.d_data), d_type(x.d_type), d_eclass(x.d_eclass) {}
	BarzerERCExpr( ) : d_type(T_LOGIC_AND ), d_eclass(EC_LOGIC) {}
	//BarzerERCExpr( uint16_t t = T_LOGIC_AND ) : d_type(t), d_eclass(EC_LOGIC) {}
	BarzerERCExpr( uint16_t t ) : d_type(t), d_eclass(EC_LOGIC) {}

	const char* getTypeName() const ;

	uint16_t getEclass() const { return d_eclass; }
	uint16_t getType() const { return d_type; }

	const DataList& getData() const { return d_data; }

	void setEclass(uint16_t t ) { d_eclass=t; }
	void setType( uint16_t t ) { d_type=t; }

    /// s in (or,and) - case insensitive. if s null - AND is default
	void setLogic( const char* s );

    template <typename T> void addClause( const T& e ) { d_data.push_back( Data(e) ); }

    template <typename T=BarzerERC>
	void addToExpr( const T& erc, uint16_t type = T_LOGIC_AND, uint16_t eclass = EC_LOGIC)
	{
		if( eclass != EC_LOGIC ) {
			d_data.push_back( Data(erc) );
			return;
		}
		if( type == d_type ) {
			d_data.push_back( Data(erc) );
		} else {
			if( d_data.empty() ) {
				d_type = type;
				d_data.push_back( Data(erc) );
			} else {
				DataList newList;
                newList.push_back( Data(*this) ) ;
                d_type = type;
                newList.push_back( Data(erc) );

                d_data.swap( newList );
			}
		}
	}
	std::ostream& print( std::ostream& fp ) const;

	bool lessThan( const BarzerERCExpr& r ) const
	{
			return ay::range_comp( ).less_than(
				d_data, d_type, d_eclass ,
				r.d_data, r.d_type, r.d_eclass
			);
	}
	bool isEqual( const BarzerERCExpr& r ) const
	{ return ( (d_data == r.d_data) && (d_type == r.d_type) && (d_eclass == r.d_eclass) ); }
};
inline bool operator< ( const BarzerERCExpr& l, const BarzerERCExpr& r )
	{ return l.lessThan(r); }
inline bool operator== ( const BarzerERCExpr& l, const BarzerERCExpr& r )
	{ return l.isEqual(r);}

struct BarzerEVR {
    BarzerEntity d_ent;
    typedef boost::variant< 
        BarzerLiteral, 
        BarzerString,
        BarzerNumber,
        BarzerDate,
        BarzerTimeOfDay,
        BarzerDateTime,
        BarzerRange,
        BarzerEntityList,
        BarzerEntity,
        BarzerERC,
        BarzerERCExpr,
        boost::optional<BarzerEVR>
    > Atom;

    typedef std::vector< std::pair<std::string,Atom> > Var;
    Var d_dta;

    const BarzerEntity& getEntity() const { return d_ent; }
    void  setEntity(const BarzerEntity& ent) { d_ent=ent; }
    const Var&  data() const { return d_dta; } 
          Var&  data() { return d_dta; } 

    decltype(d_dta)::const_iterator getIterByName( const char* name ) const
    {
        return std::find_if( d_dta.begin(), d_dta.end(), [&]( const Var::value_type& x ) { return (x.first == name); } );
    }
    decltype(d_dta)::iterator getIterByName( const char* name ) 
    {
        return std::find_if( d_dta.begin(), d_dta.end(), [&]( const Var::value_type& x ) { return (x.first == name); } );
    }
    const Atom* getAtomByName( const char* name ) const
    {
        auto x = getIterByName(name);
        return( x == d_dta.end()? 0: &(x->second) );
    }
    template <typename T>
    const Atom* getAtomContentByName( const char* name ) const
    {
        if( auto x = getAtomByName(name) ) {
            return boost::get<T>( x );
        } else 
            return 0;
    }

    void appendVar( const BarzerEVR& t )
        { d_dta.push_back( { std::string(), boost::optional<BarzerEVR>(t)} ); }

    template <typename T> void appendVar( const T& t )
        { d_dta.push_back( { std::string(), t } ); }

    ///  appends to tupple name tn
    template <typename T> void appendVar( const std::string& tag, const T& t )
        { d_dta.push_back( { tag, t } ); }

    template <typename T> void setTagVar( const std::string& tag, const T& t )
    {
        auto x = getIterByName( tag.c_str() );
        if( x == d_dta.end() ) {
            appendVar(tag,t);
        } else {
            x->second = t;
        }
    }
    void setTagVar( const std::string& tag, const BarzerEVR& t )
        { setTagVar(tag,boost::optional<BarzerEVR>(t) ); }


    void combine( const BarzerEVR& o ) 
    {
        for( auto& x : o.d_dta ) {
            if( !x.first.empty() ) {
                auto i = getIterByName( x.first.c_str() );
                if( i != d_dta.end() )  {
                    i->second = x.second;
                    continue;
                }
            } 
            d_dta.push_back( x );
        }
    }

    bool isEqual( const BarzerEVR& o ) const { return d_dta == o.d_dta; }
    bool isLessThan( const BarzerEVR& o ) const { return d_dta < o.d_dta; }
    std::ostream& print( std::ostream& fp ) const;
    
    template <typename CB>
    size_t iterateTag( const CB& cb, const char* name = 0 ) const {
        std::string s( name? name : "" );
        size_t count = 0;
        for( auto& i : d_dta ) {
            if( i.first == name ) {
                cb( i.second );
                ++count;
            }
        }
        return count;
    }
};
inline std::ostream& operator<<( std::ostream& fp, const BarzerERCExpr& x  ){ return x.print(fp); }

inline bool operator==( const BarzerEVR& x, const BarzerEVR& y ) { return x.isEqual(y); }
inline bool operator<( const BarzerEVR& x, const BarzerEVR& y ) { return x.isLessThan(y); }
} // namespace barzer 

