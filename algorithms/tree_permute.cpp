#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
using namespace std;

// bugs:
//   memory management is stupid but done this way out of laziness and to make syntax pretty
//   non-leaf nodes without children will blow it up.  Too lazy to deal with.  irrelevant.
//   I don't handle changing the tree mid-iteration.

class Node {
public:
    virtual bool step() = 0;
    virtual void yield() = 0;
    virtual ~Node() {}

    void yieldAll() {
        do {
            yield();
            cout<<endl;
        } while(step());
    }
};

class Leaf : public Node {
public:
    Leaf(char c):c_(c){}

    virtual bool step() { return false; }
    virtual void yield() { cout<<c_; }

private:
    char c_;
};
Leaf& LEAF(char c) { return *new Leaf(c); }

class Seq : public Node {
public:

    virtual bool step() {
        vector<Node*>::const_iterator iter = elements_.end();
        while(iter != elements_.begin()) {
            --iter;
            if((*iter)->step()) {
                return true;
            }
        }
        return false;
    }

    virtual void yield() {
        for(vector<Node*>::const_iterator iter = elements_.begin();
            iter != elements_.end(); ++iter) {

            (*iter)->yield();
        }
    }

    Seq& operator^(Node& element) {
        elements_.push_back(&element);
        return *this;
    }

    virtual ~Seq() {
        for(vector<Node*>::const_iterator iter = elements_.begin();
            iter != elements_.end(); ++iter) {

            delete *iter;
        }
    }

private:
    vector<Node*> elements_;
};
Seq& SEQ() { return *new Seq(); }

class Any : public Node {
public:

    bool step() {
        if(!(*position_)->step())
            ++position_;
        if(position_ == options_.end()) {
            position_ = options_.begin();
            return false;
        }
        return true;
    }

    virtual void yield() {
        (*position_)->yield();
    }

    Any& operator|(Node& option) {
        options_.push_back(&option);
        position_ = options_.begin();
        return *this;
    }

    virtual ~Any() {
        for(vector<Node*>::const_iterator iter = options_.begin();
            iter != options_.end(); ++iter) {

            delete *iter;
        }
    }

private:
    vector<Node*>::const_iterator position_;
    vector<Node*> options_;
};
Any& ANY() { return *new Any(); }

class Permute : public Node {
public:
    bool step() {
        vector<Node*>::const_iterator iter = elements_.end();
        while(iter != elements_.begin()) {
            --iter;
            if((*iter)->step()) {
                return true;
            }
        }
        return next_permutation(currentPermutation_.begin(), currentPermutation_.end());
    }

    virtual void yield() {
        for(vector<int>::const_iterator iter = currentPermutation_.begin();
            iter != currentPermutation_.end(); ++iter) {

            elements_[*iter]->yield();
        }
    }

    Permute& operator*(Node& element) {
        elements_.push_back(&element);
        currentPermutation_.push_back(currentPermutation_.size());
        return *this;
    }

    virtual ~Permute() {
        for(vector<Node*>::const_iterator iter = elements_.begin();
            iter != elements_.end(); ++iter) {

            delete *iter;
        }
    }


private:
    vector<int> currentPermutation_;
    vector<Node*> elements_;
};
Permute& PERMUTE() { return *new Permute(); }

int main() {

    Node& topNode =
        PERMUTE()
        * LEAF('a')
        * (ANY()
           | LEAF('b')
           | LEAF('c')
           | (SEQ()
              ^ LEAF('X')
              ^ (ANY()
                 | LEAF('Y')
                 | LEAF('Z'))))
        * (ANY()
           | LEAF('d')
           | LEAF('e'))
        * (SEQ()
           ^ LEAF('f')
           ^ LEAF('g'));

    topNode.yieldAll();

    delete &topNode;
}
