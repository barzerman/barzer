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
	
	NodeID_t d_lastChildId;
	NodeID_t d_nodeId;			// unique ID across its siblings
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
		n.d_nodeId = ++d_lastChildId;
        return n;
    }
    BarzelEvalNode& addChild() 
    {
        d_child.resize( d_child.size()+1 );
		auto& n = d_child.back();
		n.d_nodeId = ++d_lastChildId;
        return n;
    }
    bool getNodeId() const { return d_nodeId; } 
    void deleteChildById(NodeID_t id) {
        auto i = std::find_if( d_child.begin(), d_child.end(), 
            [&]( const BarzelEvalNode& o ) { return o.getNodeId() == id; } );
        if( i != d_child.end() )
            d_child.erase(i);
    }
	const ChildVec& getChild() const { return d_child; }
    
	const BTND_RewriteData& getBtnd() const { return d_btnd; }
	bool isFallible() const ;

    BarzelEvalNode() : d_lastChildId{0} , d_nodeId{0} {}
	BarzelEvalNode(const BTND_RewriteData& b) : d_btnd(b) , d_lastChildId{0} , d_nodeId{0} {}

	/// returns true if evaluation is successful 
	bool eval(BarzelEvalResult&, BarzelEvalContext& ctxt ) const;

    const BTND_Rewrite_Control* isComma( ) const ;
    bool equal( const BarzelEvalNode& other ) const;

    const BarzelEvalNode* getSameChild( const BarzelEvalNode& other ) const;
    BarzelEvalNode* getSameChild( const BarzelEvalNode& other ) ;
};
	
} // barzer namespace 
