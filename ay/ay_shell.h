#include <ay_util.h>
namespace ay {

class Shell {
protected:
	/// file Names 
	std::string outF, inF, errF;
	
	std::ostream outStream, errStream;
	std::istream inStream;

	virtual int interpretCmd( char_cp cmd );

public:
	typedef int (Shell::*PROCF)( char_cp cmd, std::istream& in );

	/// 
	struct CmdData {
		PROCF func;
		char_cp name; // command name
		char_cp desc; // command description
		CmdData( PROCF f, char_cp n, char_cp d) : 
			func(f), name(n), desc(d) {}
	};

	typedef <const CmdData*,const CmdData*> CmdDataRange;

	typedef char_cp_map<const CmdData*>::Type CmdDataMap;
	
	CmdDataMap cmdMap;
protected:

	// invoked from constructor . initializes cmdMap
	int indexCmdDataRange( const CmdDataRange& rng );

	inline const CmdData* getCmdDta( char_cp cmd ) const
	{
		CmdDataMap::const_iterator i = cmdMap.find( cmd );
		return( i!= cmdMap.end() ? i->second : 0 );
	}

	inline int cmdInvoke( int& rc, char_cp cmd, std::istream& in )
	{
		const CmdData* cd = getCmdData( cmd );
		return( cd ? (rc=cd->func( in ),0) : (rc=0,-1) );
	}

	CmdDataRange getProcs() const; // own procedures and fallback ones (BaseProcs)
public:
	int processOneCmd( std::istream& in );

	Shell();

	virtual int run();
};

}
