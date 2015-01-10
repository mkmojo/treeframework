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
//#include "MessageBuffer.h"

namespace treedef {
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
    //MessageBuffer m_comm;


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
    template<typename T, typename R> void BaseTree<T, R>::tree_compute(bool strict, const std::string& ComID, const std::string& EvoID, const std::string& GenID = "null"){
        //> note: needs to figure out the direction of tree compute based on generate
        if(GenID == "null"){
            if(std::string dummy_str = "some node has node.genset -> nullptr") {
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
}
#include "treedef.cpp"
