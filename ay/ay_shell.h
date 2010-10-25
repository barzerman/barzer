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
		std::wostream& print( std::wostream& ) const;
	};

	typedef std::pair<const CmdData*,const CmdData*> CmdDataRange;

	typedef wchar_cp_map<const CmdData*>::Type CmdDataMap;
	
	CmdDataMap cmdMap;
protected:

	// invoked from run . initializes cmdMap
	virtual int indexCmdDataRange( const CmdDataRange& rng );

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
	virtual int printPrompt();
private:
	// generally overloading this shouldn't be needed
	virtual int runCmdLoop();

	// *OVERLOAD this to run indexCmdDataRange with the right parms
	// by default called from run
	virtual int init();
public:
	int processOneCmd( std::wistream& in );

	Shell() : 
	outStream(0),
	errStream(0),
	inStream(0)
	{}

	// generally overloading this shouldn't be needed
	virtual int run();
};

inline std::wostream& operator <<( std::wostream& fp, const Shell::CmdData& d )
{ return d.print(fp); }

}
