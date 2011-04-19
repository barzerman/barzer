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
#include "ay/ay_logger.h"
#include "barzer_el_parser.h"
#include "barzer_config.h"
#include "barzer_el_btnd.h"
#include "barzer_universe.h"


using namespace barzer;

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

/*
<perm><any><any><perm><t>jQKDeSuFu</t></perm>
<list><n /></list>
<perm></perm>
<p>|</p>
</any>
<t>Te</t>
</any>

<list><t>V</t><p>\</p><n h="608" l="24979" /><t>HyNSC</t><any>

<perm><p>$</p><n /></perm><t>nXTAC</t><t>cPUw</t>
<opt><p>-</p><p>+</p>
</opt><t>gAA</t><p>|</p></any>

</list><p>%</p><p>;</p><n h="0.247718" l="0.534541" r="true" /></perm>
*/

int main() {
	AYLOGINIT(DEBUG);
	static StoredUniverse su;
	ay::UniqueCharPool &sp = su.getStringPool();
	BELTrie &trie = su.getBarzelTrie();
	BELPrintFormat fmt;
	BELPrintContext pctxt( trie, sp, fmt );

	BTND_PatternData pd(BTND_Pattern_Token(sp.internIt("mama")));
	BTND_Pattern_Number num; BTND_PatternData pd2(num);
	BTND_PatternData pd3(BTND_Pattern_Punct(':'));

	BELParseTreeNode tnode;
	tnode.setNodeData(BTND_StructData(BTND_StructData::T_LIST));


	tnode.addChild(pd);
	tnode.addChild(pd2);
	tnode.addChild(pd3);
	BELParseTreeNode &tnode2 = tnode.addChild(BTND_StructData(BTND_StructData::T_PERM));
	BELParseTreeNode &tnode3 = tnode2.addChild(BTND_StructData(BTND_StructData::T_ANY));
	BELParseTreeNode &tnode4 = tnode3.addChild(BTND_StructData(BTND_StructData::T_PERM));
	tnode4.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("1"))));
	tnode2.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("2"))));
	tnode2.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("3"))));
	tnode2.addChild(BTND_PatternData(BTND_Pattern_Token(sp.internIt("4"))));


	BELParseTreeNode_PatternEmitter emitter( tnode );

	//while (emitter.produceSequence()) {
	//	printPatternVec(std::cout, emitter.getCurSequence(), pctxt);
	//}

	//const BTND_PatternDataVec &vec = emitter.getCurSequence();

	//std::cout << vec.size();

    do {
		printPatternVec(std::cout, emitter.getCurSequence(), pctxt);
	} while (emitter.produceSequence()); //*/
/*
	printPatternData(std::cout, pd, pctxt) << "\n";
	printPatternData(std::cout, pd2, pctxt) << "\n";
	printPatternData(std::cout, pd3, pctxt) << "\n";
*/
	return 0;
}
