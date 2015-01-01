/** @file treedef.cpp
 *
 *  @brief This file contains implementations of base classes and default templates of the tree framework
 *
 *  @author Shule Li <shuleli@berkeley.edu> Qiyuan Qiu <qiuqiyuan@gmail.com> Saminda Wijeratne <>
 *
 *  @date 12-20-14
 *  
 *  @bug No known bugs.
 *
 *  @copyright Rochester Software License
 */

#include "treedef.hpp"
using namespace treedef;

/** @brief This function contructs a tree node. U: input data type, V: combined type
 *  @param pVar_in pointer to the input data structure
 */

template<typename U, typename V> TreeNode::TreeNode(U* pVar_in):dGen(0),dOwn(0){
    pVar = pVar_in; //< copy to shared pointer
    pGen = new NodeSet; //< initialize generate set
    pOwn = new NodeSet; //< initialize owner set
}


/** @brief This function destroys a tree node. U: input data type, V: combined type
 */
template<typename U, typename V> TreeNode::~TreeNode(){
    pGen->clear(); //< clear the generate and owner sets
    pOwn->clear();
}


/** @brief This function contructs a generate set from a user defined generate list. U: input data type, V: combined type
 *  @param list_in pointer to the input list
 *  @return void
 */
template<typename U, typename V> void TreeNode::getGenSet(NodeList* list_in){
    if(list_in->empty()) return;
    for(auto it = list_in.begin(); it != list_in.end(); it++){
        pGen->insert(*it); //< insert list nodes into the generate set
        (*it)->pOwn->insert(this); //< insert this into the list nodes' own set
        (*it)->dOwn++; //< increment the owner's counter
    }
    (*it)->dGen = list_in.size(); //< set generate counter to be the length of the list

    list_in->clear(); //< clear the list
    delete(list_in);
}


/** @brief This function contructs a generate list for one node. It needs to be specialized by the user. GenID: type of generate, T: input data type, R: combined type
 *  @param u pointer to the node being worked on
 *  @return NodeList
 */
template <std::string GenID, typename T, typename R> NodeList* Generate(int u) {
    //> default generate function for tree:
    NodeList tmplist = new NodeList; //< NodeList is std::list<int>

    //> push all children indices into the list
    for(auto it = LocalArr[u].Children.begin(); it != LocalArr[u].Children.end(); it++) //< while child exist
        tmplist->push_back(*it); 

    return tmplist;
};
   

/** @brief This function combines input data type stored in v into u. It needs to be specialized by the user. ComID: type of combine, T: input data type, R: combined type
 *  @param u pointer to the data structure storing the combined value
 *  @param v pointer to the data structure storing the input value
 *  @return void 
 */
template <std::string ComID, typename T, typename R> void Combine(R* u, T* v) {
    //>default: add all child node's input
    *u += *v;
}


/** @brief This function evolves the input data type stored in v based on combined value stored in u. It needs to be specialized by the user. EvoID: type of evolve, T: input data type, R: combined type
 *  @param u pointer to the data structure storing the combined value
 *  @param v pointer to the data structure storing the input value
 *  @return void 
 */
template<std::string EvoID, typename T, typename R> void Evolve(R* u, T* v) {
    //>default: set the new input to be the old combine
    v = u;
}


/** @brief This function adapts user defined combine into tree combine. ComID: type of combine, T: input data type, R: combined type
 *  @param u pointer to the data structure storing the combined value
 *  @param v pointer to the data structure storing the input value
 *  @return void 
 */
template<std::string ComID, typename T, typename R> void TreeCombine(int u, int v){
        Combine<ComID, T, R>(LocalArr[u].pCom, LocalArr[v].pVar);
        LocalArr[u].dGen--; //< decrease the gen counter and the own counter
        LocalArr[v].dOwn--; //< a tree node with dOwn = 0 is considered to be removed from the residual tree
}


/** @brief This function constructs a base type tree from an input data filestream. T: input data type, R: combined type
 *  @param file_in filestream pointing to input data
 */
template<typename T, typename R> BaseTree::BaseTree(std::ifstream file_in){
    auto tmp = std::getline(file_in);
    Root = new TreeNode<T,R>(tmp);
    while(tmp != nullptr){
        tmp = std::getline(file_in);
        this->Insert(tmp); //< add to tree
        TreeNodeList.pushback(tmp); //< add to nodelist
    }
}


/** @brief This function destroys a base type tree. T: input data type, R: combined type
 *  @param file_in filestream pointing to input data
 */
template<typename T, typename R> BaseTree::~BaseTree(){
    TreeNodeList.clear(); //< delete all nodes associated with this tree
}


/** @brief This function finds the common ancestor of nodes u and v. T: input data type, R: combined type
 *  @param u pointer to the first node
 *  @param v pointer to the second node
 *  @return NodePtr
 */
template<typename T, typename R> NodePtr<T,R> BaseTree::getCommonAncestor(NodePtr<T,R> u, NodePtr<T,R> v){
    //...
}


/** @brief This function creates and inserts a tree node storing input data value of data_in. T: input data type, R: combined type
 *  @param data_in pointer to the first node
 *  @return void
 */
template<typename T, typename R> void BaseTree::Insert(T* data_in){
    //...
}


/** @brief This function collapses the basetree from node u up. T: input data type, R: combined type
 *  @param u node considered
 *  @return void
 */
template<typename T, typename R> void Collapse(NodePtr<T,R> u){
    if(u->dGen != 0) return;
    auto it = u;
    while(it->parent != nullptr && it->parent->dGen <= 1){
        *(it->parent->pCom) = *(it->pCom); //< store the combined value into its parent
        Remove(it);
        it = it->parent;
    }
}


//> send tree node info indexed by u to the neighbors
template<typename T, typename R> void BaseTree:tree_send(int u){
    #ifdef(MPI)
    for(int i = 0; i < sendrequest.size(); i++){
        int proc = indexmap[-sendrequest[i]].first; 
        MPI_ISEND(&LocalArr[u], proc, MPI_COMM_TREE); //< send local array element u
    }
    #endif
}

template<typename T, typename R> void BaseTree::tree_recv(){
    #ifdef(MPI)
    auto it = ghostmap.begin();
    for(int i = 0; i < recvrequest.size(); i++){
        int proc = indexmap[-recvrequest[i]].first; 
        auto buffer = new NodePtr<T,R>;
        MPI_IRECV(buffer, proc, MPI_COMM_TREE); //< receive nodes from neighbors and store in ghost
        ghost.pushback(buffer);
        delete buffer;
        ghostmap.insert(it, std::pair<int,int>(recvrequest[i],i); //< insert the global index for reverse lookup
    }
    #endif
}

/** @brief This function do one pass of tree computation involving three steps: generate, combine and evolve. T: input data type, R: combined type
 *  @param strict if the computation has dependency on the newly combined values
 *  @param ComID type of combine
 *  @param EvoID type of evolve
 *  @param GenID type of generate: default to null
 *  @return void
 */
template<typename T, typename R> void BaseTree::tree_compute(bool strict, std::string ComID, std::string EvoID, std::string GenID = "null"){
    //> note: needs to figure out the direction of tree compute based on generate
    if(GenID == "null"){
        if(some node has node.genset -> nullptr) 
            throw UnknownGenerateSetException;
    }else{
        for(int it = 0; it < LocalArr.size(); it++){
            LocalArr[it].pGen->clear(); //< clear the generate and owner sets for all nodes before inserting
            LocalArr[it].pOwn->clear();
        }
        for(int it = 0; it < LocalArr.size(); it++){
            LocalArr[it].getGenSet(Generate<GenID, T, R>(it)); //have all gensets of nodes figured out. Note that here gen set can contain negative indices pointing to other processors
        if(strict) LocalArr.sort(); //< sort needs to be overloaded: this sorting needs to reflect how the tree nodes are removed from the residual tree
        //< note that after sorting, the indices may change. this requires all indices stores in each node to be adjusted
        //> first round combine: get all local combines done
        for(int it = 0; it < LocalArr.size(); it++){
            for(auto itt = LocalArr[it].pGen->begin(); itt != LocalArr[it].pGen->end(); itt++){
                if(*itt > 0) TreeCombine<ComID, T, R>(it, *itt, false); //< call tree combine with dependency when the required nodes are local
                //< note: necessary markings need to be done during combine
                if(!(LocalArr[it].dGen)){ //< all generates are considered for it
                    internal[it] = true;
                    if(strict) Evolve<EvoID, T, R>(it); //< update pVar with the combined value for positive dependency case
                    Collapse(it); //< collapse from *it till a node with dGen > 1
                }
            }
            if(internal[it]){
                for(auto itt = LocalArr[it].pOwn->begin(); itt != LocalArr[it].pOwn->end(); itt++)
                    if(*itt <= 0) sendrequest.pushback(-(*itt));
                tree_send(it); //< send finished nodes to neighbors
            }else{
                //> request can be reused if communication is persistant
                for(auto itt = LocalArr[it].pGen->begin(); itt != LocalArr[it].pGen->end(); itt++)
                    if(*itt <= 0) recvrequest.pushback(-(*itt));
            }
        }

        tree_recv(); //< receive node values required for combine

        //> note that this method does not collapse those chains that cross multiple processes
        for(int it = 0; it < LocalArr.size(); it++){
            if(!internal[it]){
                for(auto itt = LocalArr[it].pGen->begin(); itt != LocalArr[it].pGen->end(); it++){
                    if(*itt <= 0) TreeCombine<ComID, T, R>(it, ghostmap[*itt], true); //< combine all boundary nodes
                    if(!(LocalArr[it].dGen)){ //< all generates are considered for it
                        if(strict) Evolve<EvoID, T, R>(it); //< update pVar with the combined value for positive dependency case
                        Collapse(it); //< collapse from *it till a node with dGen > 1
                    }
                }
            }
        }

        if(!strict){
            for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++)
                Evolve<EvoID, T, R>(*it); //< update pVar with the combined value all at once for reversed or non dependency cases
        }

        sendrequest.clear();
        recvrequest.clear();
    }
}

