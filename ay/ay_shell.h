#include <ay_util.h>
#include <string>
namespace ay {

class Shell;
typedef int (*Shell_PROCF)( Shell*, char_cp cmd, std::istream& in );

struct ShellContext {
	virtual ~ShellContext() {}
};

class Shell {
protected:
	/// file Names 
	std::string outF, inF, errF;
	
	std::ostream *outStream;
	std::ostream *errStream;
	std::istream *inStream;
	ShellContext* context;

	int setupStreams();
public:
	/// individual commands 
	static int cmd_help( Shell*, char_cp cmd, std::istream& in );
	static int cmd_exit( Shell*, char_cp cmd, std::istream& in );
	// end of commands 

	/// 
	struct CmdData {
		Shell_PROCF func;
		char_cp name; // command name
		char_cp desc; // command description
		CmdData( Shell_PROCF f, char_cp n, char_cp d) : 
			func(f), name(n), desc(d) {}
		std::ostream& print( std::ostream& ) const;
	};

	typedef std::pair<const CmdData*,const CmdData*> CmdDataRange;

	typedef char_cp_map<const CmdData*>::Type CmdDataMap;
	
	CmdDataMap cmdMap;
protected:

	// invoked from run . initializes cmdMap
	virtual int indexCmdDataRange( const CmdDataRange& rng );

	inline const CmdData* getCmdDta( char_cp cmd ) const
	{
		CmdDataMap::const_iterator i = cmdMap.find( cmd );
		return( i!= cmdMap.end() ? i->second : 0 );
	}

	inline int cmdInvoke( int& rc, char_cp cmd, std::istream& in )
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
	virtual ShellContext* mkContext() { return 0; }
public:
	int processOneCmd( std::istream& in );
	ShellContext* getContext() { return context; }

	Shell() : 
		outStream(&std::cout),
		errStream(&std::cerr),
		inStream(&std::cin),
		context(0)
	{}
	virtual ~Shell() {}
	std::ostream& getOutStream() { return *outStream; }
	std::ostream& getErrStream() { return *errStream; }
	std::istream& getInStream() { return *inStream; }

	// generally overloading this shouldn't be needed
	virtual int run();
};

inline std::ostream& operator <<( std::ostream& fp, const Shell::CmdData& d )
{ return d.print(fp); }

}
