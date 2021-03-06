
/// Copyright Barzer LLC 2012
/// Code is property Barzer for authorized use only
/// 
/*
 * testme.cpp
 *
 *  Created on: Apr 10, 2011
 *      Author: polter
 */

#include <iostream>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <ay/ay_logger.h>
#include <barzer_el_parser.h>
#include <barzer_config.h>
#include <barzer_el_btnd.h>
#include <barzer_universe.h>
#include <barzer_el_function.h>
#include <barzer_basic_types.h>
#include <barzer_el_rewriter.h>
#include <ay/ay_cmdproc.h>
#include <barzer_el_btnd.h>

#include <stdio.h>

using namespace barzer;

template<class T> static std::ostream& operator<<(std::ostream &os, const std::vector<T> &vec) {
	os << "[";
	for (typename std::vector<T>::const_iterator vi = vec.begin(); vi != vec.end(); ++vi) {
		os << *vi;
		if (vi+1 < vec.end()) os << ",";
	}
	os << "]";

	return os;
}



///*
static std::ostream& printPatternData(std::ostream &os, const BTND_PatternData &pd, BELPrintContext &ctxt)
{
	switch(pd.which()) {
	case BTND_Pattern_None_TYPE:
		os << "BTND_Pattern_None_TYPE";
		//break;
		abort();
#define CASEPD(x) case BTND_##x##_TYPE: boost::get<BTND_##x>(pd).print(os, ctxt); break;
	CASEPD(Pattern_Token)
	CASEPD(Pattern_Punct)
	CASEPD(Pattern_CompoundedWord)
	CASEPD(Pattern_Number)
	CASEPD(Pattern_Wildcard)
	CASEPD(Pattern_Date)
	CASEPD(Pattern_Time)
	CASEPD(Pattern_DateTime)
#undef CASEPD
	default:
		AYLOG(ERROR) << "Unexpected pattern type\n";
	}
	return os;
}
 //*/
///*
static std::ostream& printPatternVec(std::ostream &os, const BTND_PatternDataVec &pdv,
											           BELPrintContext &ctxt)
{
	os << "[";
	for(BTND_PatternDataVec::const_iterator pi = pdv.begin(); pi != pdv.end();) {
		printPatternData(os, *pi, ctxt);
		if (++pi != pdv.end()) os << ", ";
	}
	os << "]\n";
	return os;
} //*/


static int testEmitter(StoredUniverse &su) {
	ay::UniqueCharPool &sp = su.getStringPool();
	UniverseTrieCluster &tc = su.getTrieCluster();
	BELTrie *trie = tc.getFirstTrie();
	BELPrintFormat fmt;
	BELPrintContext pctxt( *trie, sp, fmt );

	BTND_PatternData pd(BTND_Pattern_Token(sp.internIt("mama")));
	BTND_Pattern_Number num; BTND_PatternData pd2(num);
	BTND_PatternData pd3(BTND_Pattern_Punct(':'));

	BELParseTreeNode tnode;
	tnode.setNodeData(BTND_StructData(BTND_StructData::T_LIST));


	tnode.addChild(pd);
	tnode.addChild(pd2);
	tnode.addChild(pd3);


	BTND_StructData sd = BTND_StructData(BTND_StructData::T_LIST);
	sd.setVarId(6);

	BELParseTreeNode &tnode2 = tnode.addChild(sd);
	tnode2.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("1"))));
	tnode2.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("2"))));

	BTND_StructData sd2 = BTND_StructData(BTND_StructData::T_LIST);
	sd2.setVarId(7);

	BELParseTreeNode &tnode3 = tnode2.addChild(sd2);

	tnode3.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("3"))));
	tnode3.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("4"))));

	tnode.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("5"))));
	tnode.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("6"))));
	tnode.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("7"))));

//	tnode4.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("1"))));

//*/


	BELParseTreeNode_PatternEmitter emitter( tnode );

	//while (emitter.produceSequence()) {
	//	printPatternVec(std::cout, emitter.getCurSequence(), pctxt);
	//}

	//const BTND_PatternDataVec &vec = emitter.getCurSequence();

	//std::cout << vec.size();

    do {
		printPatternVec(std::cout, emitter.getCurSequence(), pctxt);
		std::cout << emitter.getVarInfo() << "\n";

	} while (emitter.produceSequence());
	return 0;
} //*/



struct PrintVisitor : boost::static_visitor<> {
	void operator()(const BarzerNumber &data) {
		data.print(std::cout) << std::endl;
	}
	void operator()(const BarzerDate &data) {
		//data.print(std::cout) << std::endl;
		printf("%02d/%02d/%04d\n", data.day, data.month, data.year);
	}
	void operator()(const BarzerTimeOfDay &data) {
		//data.print(std::cout) << std::endl;
		printf("%02d:%02d:%02d\n", data.getHH(), data.getMM(), data.getSS());
	}
	void operator()(const BarzelBeadBlank &data) {
		std::cout << "<blank />\n";
	}

	void operator()(const BarzelBeadAtomic &data) {
		boost::apply_visitor(*this, data.dta);
	}
	template<class T> void operator()(const T &data) {
		std::cout << "unknown type: ";
	}
};

static int testFunctions(StoredUniverse &su) {
	BarzelEvalResultVec vec;
	BarzelBeadAtomic bba;

	//bba.dta = BarzelBeadBlank();
	vec.resize(vec.size() + 1);
	vec.back().setBeadData(BarzelBeadBlank());



	bba.dta = BarzerNumber(10);
	vec.resize(vec.size() + 1);
	vec.back().setBeadData(bba);

	bba.dta = BarzerNumber(30);
	vec.resize(vec.size() + 1);
	vec.back().setBeadData(bba);

	bba.dta = BarzerNumber(55);
	vec.resize(vec.size() + 1);
	vec.back().setBeadData(bba);


	BarzelEvalResult res;

	su.getFunctionStorage().call("opAnd", res, vec, su);

	std::cout << res.getBeadData().which() << "\n";

	PrintVisitor pv;

	//res.setBeadData(bba);

	boost::apply_visitor(pv, res.getBeadData());

	return 0;
}

void testDateLookup(StoredUniverse &su) {
	const DateLookup dl = su.getDateLookup();
	//

	std::cout << (int) dl.lookupMonth("june") << "\n";
	std::cout << (int) dl.lookupMonth("январь") << "\n";
	std::cout << (int) dl.lookupMonth("август") << "\n";
	std::cout << (int) dl.lookupWeekday("wednesday") << "\n";
	std::cout << (int) dl.lookupWeekday("понедельник") << "\n";
}


void testSettings(StoredUniverse &su) {
	std::cout << su.getGlobalPools().getSettings().get("config.ruleset") << std::endl;
}


void testRewriteNode(barzer::GlobalPools &gp) {
    BTND_RewriteData rd1();
}


typedef std::pair<uint64_t, uint64_t> TestRange;

int main() {
	AYLOGINIT(DEBUG);
	barzer::GlobalPools gp;

	std::cout << sizeof(BTND_RewriteData) << "\n";

//	testEmitter(su);
	//testSettings(su);
	//testReader();


/*	std::cout << sizeof(BarzerNumber)
		//<< " vs " << sizeof(BarzerDate)
		//<< " vs " << sizeof(BarzerTimeOfDay)
		//<< " vs " << sizeof(BarzerRange)
		//<< " vs " << sizeof(TestRange)
		<< " vs " << sizeof(TestNumber)
		<< "\n";
		//*/

	//return testFunctions(su);



	return 0;
}
