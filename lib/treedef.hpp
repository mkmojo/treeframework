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

namespace treedef {

    //> type declarations and short hands
    template <typename U, typename V> class TreeNode;
    template <typename U, typename V> using NodePtr = std::shared_ptr<TreeNode<U, V> >; //< this typedef only works for 0x
    template <typename U, typename V> using NodeList = std::list<std::shared_ptr<TreeNode<U, V> > >; //< this typedef only works for 0x
    template <typename U, typename V> using NodeSet = std::set<std::shared_ptr<TreeNode<U, V> > >; //< this typedef only works for 0x
    template <std::string GenID, typename T, typename R> NodeList* Generate(NodePtr<T, R>);
    template <std::string ComID, typename T, typename R> void Combine(R*, T*);
    template <std::string EvoID, typename T, typename R> void Evolve(R*, T*);
    template <std::string ComID, typename T, typename R> void TreeCombine(NodePtr<T,R>, NodePtr<T,R>);
    template <typename T, typename R> class BaseTree;

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
        typedef std::shared_ptr<typename U> VarPtr;
        typedef std::shared_ptr<typename V> ComPtr;

    public:
        VarPtr pVar; //< pointer to the input field of a tree node
        ComPtr pCom; //< pointer to the combined field of a tree node
        NodeSet *pGen, *pOwn; //< pointer to the generate set and owner set
        int dGen, dOwn;  //< generate and owner counter for the residual tree 
        NodePtr<T, R> left, parent, sibling; //< pointer to the neighbors

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
        std::vector<NodePtr<T,R> >  ghost; //< stores ghost nodes
        std::map<int,int> indexmap, ghostmap; //< maps local index to global index and vice versa
        std::vector<int> sendrequest, recvrequest; 
        NodePtr<T, R> getCommonAncestor(NodePtr<T,R> u, NodePtr<T,R> v); //< get common ancestors of nodes u and v
        void Insert(T* data_in); //< insert node with input value stored in data_in
        void Remove(NodePtr<T,R> u); //< remove node u from the residual graph
        void Collapse(NodePtr<T,R> u); //< collapse node u upward the tree
        NodeList<T,R> TreeNodeList;  //< stores the entire list of the tree nodes. in case of array implementation, this should be a vector instead of list
        void tree_send(int); 
        void tree_recv();

    public: 
        BaseTree(std::ifstream); //< constructs a base tree from an input filestream 
        virtual ~BaseTree(); //< destructor. needs overriding
        void tree_compute(bool, std::string, std::string, std::string); //< calls generate or combine templates. specialized tree_compute can be implemented for different types of trees by overriding
    };

}

