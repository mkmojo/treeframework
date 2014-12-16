#include "treedef.hpp"
using namespace treedef;

//> TreeNode constructor
template<typename U, typename V> TreeNode::TreeNode(U* pVar_in){
    pVar = pVar_in; //< copy as shared pointer
    pGen = new GenSet; //< initialize generate set
}


//> TreeNode destructor
template<typename U, typename V> TreeNode::~TreeNode(){
    pGen->clear();
}


//> Insert members of NodeList into generate set
template<typename U, typename V> void TreeNode::getGenSet(NodeList* list_in){
    if(list_in->empty()) return;
    for(auto it = list_in.begin(); it != list_in.end(); it++)
        pGen->insert(*it);
    list_in->clear();
    delete(list_in);
}


//> Function template for Generate
template <std::string GenID, typename T, typename R> NodeList* Generate(NodePtr<T, R> u) {
    //default generate function for tree:
    auto pChild = u->left;
    auto tmplist = new std::list<NodePtr>();

    do{
        tmplist->push_back(pChild);
    } while(pChild.sibling != nullptr)
    return tmplist;
};
   

//> Function template for Combine
template <std::string ComID, typename T, typename R> void Combine(NodePtr<T,R> u) {
    //default combine function for tree:
    if(u->genset == nullptr) 
        throw TreeException("Unknown Generate Set in Combine"); //exception: generate not specified

    T tmp; 
    for(auto it = u->pGen->begin(); it != u->pGen->end(); it ++)
        tmp += *(tmp->pVar);

    *(u->pCom) = tmp;
}


//> Find common ancestors of nodes u and v
template<typename T, typename R> NodePtr BaseTree::getCommonAncestor(NodePtr u, NodePtr v){
    //...
}


//> insert a new node with data value data_in
template<typename T, typename R> void BaseTree::Insert(T* data_in){
    //...
}


//> BaseTree constructor: construct a tree from data file
template<typename T, typename R> BaseTree::BaseTree(std::ifstream file_in){
    auto tmp = std::getline(file_in);
    Root = new TreeNode(tmp);
    while(tmp != nullptr){
        tmp = std::getline(file_in);
        this->Insert(tmp); //< add to tree
        TreeNodeList.pushback(tmp); //< add to nodelist
    }
}
        

//> Call Generate and Combine on the tree
template<typename T, typename R> void BaseTree::tree_compute(bool strict, std::string ComID, std::string GenID = "null"){
    if(GenID == "null"){
        if(some node has node.genset -> nullptr) 
            throw UnknownGenerateSetException;
    }else{
        for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++)
            *it->getGenSet(Generate<GenID, T, R>(*it)); //have all gensets of nodes figured out
        TreeNodeList.sort();
        for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++)
            Combine<ComID, T, R>(*it);
    }
}

//> Call Evolution function for the tree
template<typename T, typename R> void BaseTree::tree_evolve(std::string EvoID){
    //evolve the input data to the next iteration
}


