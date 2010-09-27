#include <ay_util.h>
namespace ay {

class Shell {
protected:
	/// file Names 
	std::string outF, inF, errF;
	
	std::ofstream *outStream, *errStream;
	std::ifstream *inStream;

	void setUpStreams();
public:
	/// individual commands 
	int cmd_help( wchar_cp cmd, std::wistream& in );
	int cmd_exit( wchar_cp cmd, std::wistream& in );
	// end of commands 
	typedef int (Shell::*PROCF)( wchar_cp cmd, std::wistream& in );

	/// 
	struct CmdData {
		PROCF func;
		wchar_cp name; // command name
		wchar_cp desc; // command description
		CmdData( PROCF f, wchar_cp n, wchar_cp d) : 
			func(f), name(n), desc(d) {}
	};

	typedef std::pair<const CmdData*,const CmdData*> CmdDataRange;

	typedef wchar_cp_map<const CmdData*>::Type CmdDataMap;
	
	CmdDataMap cmdMap;
protected:

	// invoked from constructor . initializes cmdMap
	int indexCmdDataRange( const CmdDataRange& rng );

	inline const CmdData* getCmdDta( wchar_cp cmd ) const
	{
		CmdDataMap::const_iterator i = cmdMap.find( cmd );
		return( i!= cmdMap.end() ? i->second : 0 );
	}

	inline int cmdInvoke( int& rc, wchar_cp cmd, std::wistream& in )
	{
		const CmdData* cd = getCmdData( cmd );
		return( cd ? (rc=(this->*(cd->func))( in ), 0) : (rc=0,-1) );
	}

	CmdDataRange getProcs() const; // own procedures and fallback ones (BaseProcs)

public:
	int processOneCmd( std::wistream& in );

	Shell();

	virtual int run();
};

}
