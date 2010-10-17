#include <ay_util.h>
#include <string>
namespace ay {

class Shell;
typedef int (*Shell_PROCF)( Shell*, wchar_cp cmd, std::wistream& in );

class Shell {
protected:
	/// file Names 
	std::string outF, inF, errF;
	
	std::wostream *outStream;
	std::wostream *errStream;
	std::wistream *inStream;

	int setupStreams();
public:
	/// individual commands 
	static int cmd_help( Shell*, wchar_cp cmd, std::wistream& in );
	static int cmd_exit( Shell*, wchar_cp cmd, std::wistream& in );
	// end of commands 

	/// 
	struct CmdData {
		Shell_PROCF func;
		wchar_cp name; // command name
		wchar_cp desc; // command description
		CmdData( Shell_PROCF f, wchar_cp n, wchar_cp d) : 
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
		const CmdData* cd = getCmdDta( cmd );
		return( cd ? (rc=cd->func( this, cmd, in ), 0) : (rc=0,-1) );
	}

	CmdDataRange getProcs() const; // own procedures and fallback ones (BaseProcs)

public:
	int processOneCmd( std::wistream& in );

	Shell();

	virtual int run();
};

}
