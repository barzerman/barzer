#pragma once 
namespace barzer {
class BarzelEvalResult;
class BarzelEvalContext;

class BarzelEvalNode {
public:
    typedef std::vector< BarzelEvalNode > ChildVec;
	
	typedef uint16_t NodeID_t;
protected:
	BTND_RewriteData d_btnd;

	ChildVec d_child;
	//// will recursively add nodes 
	
	NodeID_t m_lastChildId;
	NodeID_t m_thisSiblingId;			// unique ID across its siblings
public:
	typedef std::pair< const uint8_t*, const uint8_t* > ByteRange;
private:
	bool isSubstitutionParm( size_t& pos ) const;
	bool eval_comma(BarzelEvalResult&, BarzelEvalContext& ctxt ) const;
public:
    // returns true if this node if this is a LET node - let node assigns a variable 
    const BTND_Rewrite_Control* getControl() const 
        { return boost::get<BTND_Rewrite_Control>(&d_btnd); }
    void setBtnd( const BTND_RewriteData& rd ) 
        { d_btnd= rd; }
	BTND_RewriteData& getBtnd() { return d_btnd; }
	ChildVec& getChild() { return d_child; }
	
	NodeID_t getSiblingId() const { return m_thisSiblingId; }

    /// takes o's children and adds the ones that are new as children
    /// children of o which already appear in d_child are not added.
    /// returns the number of actual children added
    std::vector<NodeID_t> addNodesChildren(const BarzelEvalNode& o) ;

    /// returns true if o's children are the same as this node's children  
    bool hasSameChildren( const BarzelEvalNode& o) const;
    
    // adds this node as a child
    BarzelEvalNode& addChild(const BarzelEvalNode& o) 
    {
        d_child.push_back(o);
		auto& n = d_child.back();
		n.m_thisSiblingId = ++m_lastChildId;
        return n;
    }
    BarzelEvalNode& addChild() 
    {
        d_child.resize( d_child.size()+1 );
		auto& n = d_child.back();
		n.m_thisSiblingId = ++m_lastChildId;
        return n;
    }

	const ChildVec& getChild() const { return d_child; }

	const BTND_RewriteData& getBtnd() const { return d_btnd; }
	bool isFallible() const ;
	BarzelEvalNode()
	: m_lastChildId{0}
	, m_thisSiblingId{0}
	{}
	
	BarzelEvalNode(const BTND_RewriteData& b)
	: d_btnd(b)
	, m_lastChildId{0}
	, m_thisSiblingId{0}
	{}

	/// returns true if evaluation is successful 
	bool eval(BarzelEvalResult&, BarzelEvalContext& ctxt ) const;

    const BTND_Rewrite_Control* isComma( ) const ;

    bool equal( const BarzelEvalNode& other ) const;

    const BarzelEvalNode* getSameChild( const BarzelEvalNode& other ) const;
    BarzelEvalNode* getSameChild( const BarzelEvalNode& other ) ;
};
	
} // barzer namespace 
