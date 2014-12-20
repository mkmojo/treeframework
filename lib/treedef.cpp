#include "treedef.hpp"
using namespace treedef;

//> TreeNode constructor
template<typename U, typename V> TreeNode::TreeNode(U* pVar_in):dGen(0),dOwn(0){
    pVar = pVar_in; //< copy as shared pointer
    pGen = new NodeSet<U, V>; //< initialize generate set
    pOwn = new NodeSet<U, V>; //< initialize owner set
}


//> TreeNode destructor
template<typename U, typename V> TreeNode::~TreeNode(){
    pGen->clear();
}


//> Insert members of NodeList into this node's generate set
template<typename U, typename V> void TreeNode::getGenSet(NodeList<U,V>* list_in){
    if(list_in->empty()) return;
    for(auto it = list_in.begin(); it != list_in.end(); it++){
        pGen->insert(*it); //< insert list nodes into my genset
        (*it)->pOwn->insert(this); //< insert myself into the list nodes' ownset
        (*it)->dOwn++; //< increment the owner's counter
    }
    (*it)->dGen = list_in.size(); //< set dGen to be the length of the list

    list_in->clear();
    delete(list_in);
}


//> Function template for Generate
template <std::string GenID, typename T, typename R> NodeList<T,R>* Generate(NodePtr<T, R> u) {
    //> default generate function for tree:
    auto pChild = u->left;
    auto tmplist = new NodeList<T, R>;

    //> push all children into the list
    do{
        tmplist->push_back(pChild);
    } while(pChild.sibling != nullptr)
    return tmplist;
};
   

//> Function template for Combine: combine v's input value with u's and store it in u's comptr
//> Node v is then removed from the residual tree
template <std::string ComID, typename T, typename R> void Combine(R* u, T* v) {
    //>default: add all child node's input
    *u += *v;
}


//> Function template for evolving one node: update input field v using combined value u
template<std::string EvoID, typename T, typename R> void Evolve(R* u, T* v) {
    //>default: set the new input to be the old combine
    v = u;
}


//> adapter for user defined combine
template<std::string ComID, typename T, typename R> void TreeCombine(NodePtr<T,R> u, NodePtr<T,R> v){
        Combine<ComID, T, R>(u->pCom, v->pVar);
        u->dGen--; //< decrease the gen counter and the own counter
        v->dOwn--; //< a tree node with dOwn = 0 is considered to be removed from the residual tree
}


//> BaseTree constructor: construct a tree from data file
template<typename T, typename R> BaseTree::BaseTree(std::ifstream file_in){
    auto tmp = std::getline(file_in);
    Root = new TreeNode<T,R>(tmp);
    while(tmp != nullptr){
        tmp = std::getline(file_in);
        this->Insert(tmp); //< add to tree
        TreeNodeList.pushback(tmp); //< add to nodelist
    }
}


//> BaseTree destructor
template<typename T, typename R> BaseTree::~BaseTree(){
    TreeNodeList.clear(); //< delete all nodes associated with this tree
}


//> Find common ancestors of nodes u and v
template<typename T, typename R> NodePtr<T,R> BaseTree::getCommonAncestor(NodePtr<T,R> u, NodePtr<T,R> v){
    //...
}


//> insert a new node with data value data_in
template<typename T, typename R> void BaseTree::Insert(T* data_in){
    //...
}
       


//> figure out the direction of tree compute based on generate
//> how is the tree collapse called inbetween combines: let user define or framework should be smart enough to do that 
//> have a counter in each node = initial degree. decrease the counter per combine. once zero, traverse up the tree, collapse edges that has parent of counter = 1.
//> array implementation of the tree. marking are collapsing points of upward
//> Call Generate and Combine on the tree
template<typename T, typename R> void BaseTree::tree_compute(bool strict, std::string ComID, std::string EvoID, std::string GenID = "null"){
    if(GenID == "null"){
        if(some node has node.genset -> nullptr) 
            throw UnknownGenerateSetException;
    }else{
        for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++){
            (*it)->pGen->clear(); //< clear the generate and owner sets for all nodes before inserting
            (*it)->pOwn->clear();
        }
        for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++)
            (*it)->getGenSet(Generate<GenID, T, R>(*it)); //have all gensets of nodes figured out
        if(strict) TreeNodeList.sort(); //< sort needs to be overloaded: this sorting needs to reflect how the tree nodes are removed from the residual tree
        for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++){
            for(auto itt = (*it)->pGen.begin(); itt != (*it)->pGen.end(); itt++){
                TreeCombine<ComID, T, R>(*it, *itt); //< call tree combine with dependency
                if(!(*it->dGen)){ //< it becomes leaf
                    if(strict) Evolve<EvoID, T, R>(*it); //< update pVar with the combined value for positive dependency case
                    tree_collapse(*it); //< collapse from *it till a node with dGen > 1
                }
            }
        }
        if(!strict){
            for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++)
                Evolve<EvoID, T, R>(*it); //< update pVar with the combined value all at once for reversed or non dependency cases
        }
    }
}

//> Call Evolution function for the tree
template<typename T, typename R> void BaseTree::tree_evolve(std::string EvoID){
    //evolve the input data to the next iteration
}


