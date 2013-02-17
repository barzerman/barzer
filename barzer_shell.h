
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
#pragma once
#include <ay/ay_headers.h>
#include <ay/ay_shell.h>
#include <barzer_dtaindex.h>
#include <set>
#include <barzer_parse.h>
//#include <barzer_parse_types.h>
#include <barzer_barz.h>
#include <barzer_universe.h>
#include <barzer_el_trie_walker.h>
#include <barzer_el_wildcard.h>
#include <barzer_server_response.h>
#include <barzer_server.h>
#include <zurch_docidx.h>


#include <iostream>
namespace barzer {

//struct BarzerShellContext;


struct BarzerShellContext : public ay::ShellContext {
	GlobalPools& gp;
	StoredUniverse* d_universe;
    
    std::string d_queryRet;

    zurch::DocIndexAndLoader       d_zurchFS;

	BELTrieWalker trieWalker;
	BELTrie* d_trie;

	Barz barz;
	QParser parser;
    RequestEnvironment reqEnv;

    int d_grammarId;
    BarzStreamerXML::ModeFlags streamerModeFlags_bits;
    ay::named_bits< BarzStreamerXML::ModeFlags > streamerModeFlags;

	DtaIndex* obtainDtaIdx() { return &(d_universe->getDtaIdx()); }
	BELTrie& getTrie() { return *d_trie; }
	const BELTrie& getTrie() const { return *d_trie; }

	BarzerShellContext(StoredUniverse& u, BELTrie& trie);

	BELTrie* getCurrentTriePtr() { return d_trie; }
	void setTrie( BELTrie* t ) 
	{
		if( t ) 
			d_trie = t;
		else {
			std::cerr << "trying to set null trie in barzer shell context\n";
		}
	}
	GlobalPools& getGLobalPools() { return gp; }
	const GlobalPools& getGLobalPools() const { return gp; }

	StoredUniverse& getUniverse() { return *d_universe; }
	const StoredUniverse& getUniverse() const { return *d_universe; }

	void setUniverse( StoredUniverse* u ) { d_universe= u; }
	StoredUniverse* getUserUniverse( uint32_t uid ) { return gp.getUniverse(uid); }
	bool userExists( uint32_t uid ) const { return (gp.getUniverse( uid ) != 0); }
	bool isUniverseValid() const { return (d_universe!= 0 ); }
};
struct BarzerShellEnv {
    int  stemMode; // name - "stemMode" QuestionParm::STEMMODE_XXX
    BarzerShellEnv(): stemMode(QuestionParm::STEMMODE_NORMAL) {}

    /// prints result of the setting to the stream
    void set( std::ostream&, const char* n, const char* v ) ;
    // when n == 0 prints all valid settings 
    std::ostream&  get( std::ostream&, const char* n=0 ) const;
};

struct BarzerShell : public ay::Shell {
	uint32_t d_uid; // user id 
	GlobalPools& gp;

    BarzerShellEnv shellEnv;

	/// if forceCreate is  true user will be created even if it doesnt exist
	/// otherwise the function will attempt to retrieve the universe and report error
	/// in case of a failure 
	int setUser( uint32_t uid, bool forceCreate  );

	BarzerShell( uint32_t uid, GlobalPools& g ) : d_uid(uid), gp(g) {}
    uint32_t getUser() const { return d_uid; }
	virtual int init( );
	virtual ay::ShellContext* mkContext();
	virtual ay::ShellContext* cloneContext();
	virtual BarzerShell* cloneShell() 
	{
		BarzerShell* newShell =  new BarzerShell( *this );
		ay::ShellContext* newCtxt = this->cloneContext();
		newShell->setContext( newCtxt );
		return newShell;
	}

	BarzerShellContext* getBarzerContext() ;
    void syncQuestionParm( QuestionParm& qparm );
};

} // namespace barzer
