
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <ay_headers.h>
#include <ay_util.h>
#include <ay_bitflags.h>
#include <string>
#include <boost/thread.hpp>

namespace ay {

class Shell;
typedef int (*Shell_PROCF)( Shell*, char_cp cmd, std::istream& in, const std::string& str );

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
    std::vector< boost::thread* > d_bgThreadvec;
public:
    auto bgThreads( ) -> decltype(d_bgThreadvec)& { return d_bgThreadvec; }
    auto bgThreads( ) const -> const decltype(d_bgThreadvec)& { return d_bgThreadvec; }

    bool isStream_cerr( ) const { return errStream == &std::cerr; }
    bool isStream_cout( ) const { return outStream == &std::cout; }
    bool isStream_cin( ) const { return inStream == &std::cin; }
    enum {
        SHBIT_NOPROMPT, // when on neither prompt nor help is not printed 
        SHBIT_MAX
    };
    ay::bitflags< SHBIT_MAX > d_bit;

    const std::string& outFileName() const { return outF; }
    const std::string& inFileName() const { return inF; }
    const std::string& errFileName() const { return errF; }
    
    std::ostream& printStreamState( std::ostream& fp ) const;
    // default stdin
    std::istream& setInFile( const char* fn=0 );
    // default stdout
    std::ostream& setOutFile( const char* fn=0 );
    // default stderr
    std::ostream& setErrFile( const char* fn=0 );

	bool echo; /// when on prints commands to std:cerr 
	/// individual commands 
	static int cmd_set( Shell*, char_cp cmd, std::istream& in, const std::string&  );
	static int cmd_run( Shell*, char_cp cmd, std::istream& in, const std::string&  );
	static int cmd_help( Shell*, char_cp cmd, std::istream& in, const std::string&  );
	static int cmd_exit( Shell*, char_cp cmd, std::istream& in, const std::string&  );
	static int cmd_quit( Shell*, char_cp cmd, std::istream& in, const std::string&  );
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
	virtual int indexCmdDataRange( const CmdDataRange& rng, bool reportDups = false );

	inline const CmdData* getCmdDta( char_cp cmd ) const
	{
		CmdDataMap::const_iterator i = cmdMap.find( cmd );
		return( i!= cmdMap.end() ? i->second : 0 );
	}
    // background - if true command is launched in its own thread, shell is cloned 
	inline int cmdInvoke( int& rc, char_cp cmd, std::istream& in, const std::string& argStr, bool background );

	CmdDataRange getProcs() const; // own procedures and fallback ones (BaseProcs)
	virtual int printPrompt();
private:
	// generally overloading this shouldn't be needed
	// when fp is 0 inStream is assumed
	virtual int runCmdLoop(std::istream* fp =0, const char* runScript = 0);

	virtual ShellContext* mkContext() { return 0; }
	virtual ShellContext* cloneContext() { return 0; }
public:
    int indexDefaultCommands();

	// *OVERLOAD this to run indexCmdDataRange with the right parms
	// by default called from run
	virtual int init();
	Shell& setContext( ShellContext* ctxt ) { if( context ) delete context; context = ctxt; return *this;}

	int processOneCmd( std::istream& in );
	ShellContext* getContext() { return context; }

	Shell(const Shell& sh ) : 
		outStream(sh.outStream),
		errStream(sh.errStream),
		inStream(sh.inStream),
		context(0),
		echo(sh.echo)
	{}
	Shell() : 
		outStream(&std::cout),
		errStream(&std::cerr),
		inStream(&std::cin),
		context(0),
		echo(false)
	{}
	virtual Shell* cloneShell() { return new Shell(*this); }

	virtual ~Shell();

    std::istream* setInputStream( std::istream* p ) { 
        std::istream* old = inStream;
        return( p ? ( inStream=p, old) : 0 );
    }
    std::ostream* setOutStream( std::ostream* p ) { 
        std::ostream* oldStream = outStream;
        return( p ? ( outStream=p, oldStream) : 0 );
    }
	std::ostream* getOutStreamPtr() { return outStream; }
	std::ostream& getOutStream() { return *outStream; }
	std::ostream* getErrStreamPtr() { return errStream; }
	std::ostream& getErrStream() { return *errStream; }
	std::istream* getInStreamPtr() { return inStream; }
	std::istream& getInStream() { return *inStream; }

	// generally overloading this shouldn't be needed
    // if exec runScript !=0 tries to load file from 'runScript' and run commands in it
	virtual int run( const char* runScript = 0);
};

inline std::ostream& operator <<( std::ostream& fp, const Shell::CmdData& d )
{ return d.print(fp); }

}
