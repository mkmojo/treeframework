#ifndef TREEDEF_H
#define TREEDEF_H 1
/** @file treedef.hpp
 *
 *  @brief This header file contains base classes and default template definitions to use the tree framework
 *
 *  @author Shule Li <shuleli@berkeley.edu> Qiyuan Qiu <qiuqiyuan@gmail.com> Saminda Wijeratne <>
 *
 *  @date 12-20-14
 *  
 *  @bug No known bugs.
 *
 *  @copyright Rochester Software License
 */

#include <string>
#include <set>
#include <list>
#include <memory>
#include <exception>
#include <vector>
#include <fstream>
#include <iostream>
#include "MessageBuffer.h"

namespace treedef {
    enum NodeFlag // < mark to delete node
    {
        NF_DELETE = 0x1,
    };
    //> type declarations and short hands
    template <typename U, typename V> class TreeNode;
    template <typename U, typename V> using NodePtr = std::shared_ptr<TreeNode<U, V> >; //< this typedef only works for 0x
    template <typename U, typename V> using NodeList = std::vector<std::shared_ptr<TreeNode<U, V> > >; //< this typedef only works for 0x
    template <typename U, typename V> using NodeSet = std::set<std::shared_ptr<TreeNode<U, V> > >; //< this typedef only works for 0x
    template <const std::string& GenID, typename T, typename R> NodeList<T, R>* Generate(NodePtr<T, R>);
    template <const std::string& ComID, typename T, typename R> void Combine(T*, R*);
    template <const std::string& EvoID, typename T, typename R> void Evolve(T*, R*);
    template <const std::string& ComID, typename T, typename R> void TreeCombine(NodePtr<T,R>, NodePtr<T,R>);
    template <typename T, typename R> class BaseTree;
    //MessageBuffer m_comm; // < comunication layer


    /** @brief This class stores customized error message
     *  can be derived further to include more detailed error
     */
    class TreeException : public std::runtime_error{
        public:
            TreeException(std::string const& message):std::runtime_error("Tree Error: "+message+" was Thrown.\n"){};

    };

    /** @brief This class stores one tree node
     *  @param U input data type
     *  @param V combined data type
     */
    template <typename U, typename V> class TreeNode {
        typedef std::shared_ptr<U> VarPtr;
        typedef std::shared_ptr<V> ComPtr;

        public:
        VarPtr pVar; //< pointer to the input field of a tree node
        ComPtr pCom; //< pointer to the combined field of a tree node
        NodeSet<U, V> *pGen, *pOwn; //< pointer to the generate set and owner set
        int dGen, dOwn;  //< generate and owner counter for the residual tree 
        NodePtr<U, V> left, parent, sibling; //< pointer to the neighbors

        TreeNode(U*); //< constructor: a pointer to the initial input field value is passed in
        ~TreeNode(); //< destructor
        void getGenSet(NodeList<U, V>*); //< converts a list of node pointers into generate set
    };

    /** @brief This class stores a base type tree. Different types of trees can be derived from this.
     *  @param T input data type
     *  @param R combined data type
     */
    template <typename T, typename R> class BaseTree {
        protected:
            NodePtr<T, R> Root; //< pointer to the root
            NodePtr<T, R> getCommonAncestor(NodePtr<T,R> u, NodePtr<T,R> v); //< get common ancestors of nodes u and v
            void Insert(T* data_in); //< insert node with input value stored in data_in
            void Remove(NodePtr<T,R> u); //< remove node u from the residual graph
            void Collapse(NodePtr<T,R> u); //< collapse node u upward the tree
            NodeList<T,R> TreeNodeList;  //< stores the entire list of the tree nodes. in case of array implementation, this should be a vector instead of list

        public: 
            BaseTree(){}; //< constructs a base tree from an input filestream 
            ~BaseTree(){}; //< destructor. needs overriding
            void tree_compute(bool, const std::string&, const std::string&, const std::string&); //< calls generate or combine templates. specialized tree_compute can be implemented for different types of trees by overriding
    };

    /** @brief This function do one pass of tree computation involving three steps: generate, combine and evolve. T: input data type, R: combined type
     *  @param strict if the computation has dependency on the newly combined values
     *  @param ComID type of combine
     *  @param EvoID type of evolve
     *  @param GenID type of generate: default to null
     *  @return void
     */
    template<typename T, typename R> void BaseTree<T, R>::tree_compute(bool strict, const std::string& ComID, const std::string& EvoID, const std::string& GenID){
        //> note: needs to figure out the direction of tree compute based on generate
        if(GenID == "null"){
            if(false) {
                //std::string dummy_str = "some node has node.genset -> nullptr"
                //throw UnknownGenerateSetException;
            }
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
                    //< note: necessary markings need to be done during combine
                    if(!(*it->dGen)){ //< all generates are considered for it
                        if(strict) Evolve<EvoID, T, R>(*it); //< update pVar with the combined value for positive dependency case
                        Collapse(*it); //< collapse from *it till a node with dGen > 1
                    }
                }
            }
            if(!strict){
                for(auto it = TreeNodeList.begin(); it != TreeNodeList.end(); it++)
                    Evolve<EvoID, T, R>(*it); //< update pVar with the combined value all at once for reversed or non dependency cases
            }
        }
    }

    /** @brief This function contructs a tree node. U: input data type, V: combined type
     *  @param pVar_in pointer to the input data structure
     */
    template <typename U, typename V> 
        TreeNode<U, V>::TreeNode(U* pVar_in):dGen(0),dOwn(0) {
            pVar = pVar_in; //< copy to shared pointer
            pGen = new NodeSet<U, V>; //< initialize generate set
            pOwn = new NodeSet<U, V>; //< initialize owner set
        }


    /** @brief This function destroys a tree node. U: input data type, V: combined type
    */
    template<typename U, typename V> 
        TreeNode<U,V>::~TreeNode(){
            pGen->clear(); //< clear the generate and owner sets
            pOwn->clear();
        }


    /** @brief This function contructs a generate set from a user defined generate list. U: input data type, V: combined type
     *  @param list_in pointer to the input list
     *  @return void
     */
    template<typename U, typename V> 
        void TreeNode<U,V>::getGenSet(NodeList<U, V>* list_in){
            if(list_in->empty()) return;
            //for(auto it = list_in.begin(); it != list_in.end(); it++){
            //    this->pGen->insert(*it); //< insert list nodes into the generate set
            //    (*it)->pOwn->insert(this); //< insert this into the list nodes' own set
            //    (*it)->dOwn++; //< increment the owner's counter
            //}
            //this->dGen = list_in.size(); //< set generate counter to be the length of the list

            list_in->clear(); //< clear the list
            delete(list_in);
        }
    /** @brief This function contructs a generate list for one node. It needs to be specialized by the user. GenID: type of generate, T: input data type, R: combined type
     *  @param u pointer to the node being worked on
     *  @return NodeList
     */
    template <const std::string& GenID, typename T, typename R> NodeList<T,R>* Generate(NodePtr<T, R> u) {
        //> default generate function for tree:
        auto pChild = u->left;
        auto tmplist = new NodeList<T, R>;

        //> push all children into the list
        do{
            tmplist->push_back(pChild);
        } while(pChild.sibling != nullptr);
        return tmplist;
    }


    /** @brief This function combines input data type stored in v into u. It needs to be specialized by the user. ComID: type of combine, T: input data type, R: combined type
     *  @param u pointer to the data structure storing the combined value
     *  @param v pointer to the data structure storing the input value
     *  @return void 
     */
    template <const std::string& ComID, typename T, typename R> void Combine(R* u, T* v) {
        //>default: add all child node's input
        *u += *v;
    }


    /** @brief This function evolves the input data type stored in v based on combined value stored in u. It needs to be specialized by the user. EvoID: type of evolve, T: input data type, R: combined type
     *  @param u pointer to the data structure storing the combined value
     *  @param v pointer to the data structure storing the input value
     *  @return void 
     */
    template<const std::string& EvoID, typename T, typename R> void Evolve(R* u, T* v) {
        //>default: set the new input to be the old combine
        v = u;
    }


    /** @brief This function adapts user defined combine into tree combine. ComID: type of combine, T: input data type, R: combined type
     *  @param u pointer to the data structure storing the combined value
     *  @param v pointer to the data structure storing the input value
     *  @return void 
     */
    template<const std::string& ComID, typename T, typename R> void TreeCombine(NodePtr<T,R> u, NodePtr<T,R> v){
        Combine<ComID, T, R>(u->pCom, v->pVar);
        u->dGen--; //< decrease the gen counter and the own counter
        v->dOwn--; //< a tree node with dOwn = 0 is considered to be removed from the residual tree
    }


    /** @brief This function finds the common ancestor of nodes u and v. T: input data type, R: combined type
     *  @param u pointer to the first node
     *  @param v pointer to the second node
     *  @return NodePtr
     */
    template<typename T, typename R> NodePtr<T,R> BaseTree<T, R>::getCommonAncestor(NodePtr<T,R> u, NodePtr<T,R> v){
        //...
    }


    /** @brief This function creates and inserts a tree node storing input data value of data_in. T: input data type, R: combined type
     *  @param data_in pointer to the first node
     *  @return void
     */
    template<typename T, typename R> void BaseTree<T, R>::Insert(T* data_in){
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

    template <typename T, typename R> class OctTree : public BaseTree<T, R>{
        //implement specialized methods for OctTree
        public:
            T setCounter(T);
            OctTree();
            ~OctTree();
        private:
            T counter;
    };
    template <typename T, typename R> OctTree<T,R>::OctTree():counter(T()){
        std::cout << "OctTree constructor" << std::endl;

        // auto tmp = std::getline(file_in);
        // Root = new TreeNode<T,R>(tmp);
        // while(tmp != nullptr){
        //     tmp = std::getline(file_in);
        //     this->Insert(tmp); //< add to tree
        //     TreeNodeList.pushback(tmp); //< add to nodelist
        // }
    }

    template <typename T, typename R>
        OctTree<T,R>::~OctTree(){
            std::cout<< " OctTree destructor " << std::endl;
        }


    template <typename T, typename R> 
        T OctTree<T,R>::setCounter(T val){  
            counter = val;
            return counter;
        }
}
#endif
