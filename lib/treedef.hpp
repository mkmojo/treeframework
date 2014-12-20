//defines the tree interface
#include <string>
#include <set>
#include <list>
#include <memory>
#include <exception>

namespace treedef {

    template<typename U, typename V> class TreeNode;
    template<typename U, typename V> using NodePtr = std::shared_ptr<TreeNode<U, V> >;
    template<typename U, typename V> using NodeList = std::list<std::shared_ptr<TreeNode<U, V> > >;
    template<typename U, typename V> using NodeSet = std::set<std::shared_ptr<TreeNode<U, V> > >;
    template<std::string GenID, typename T, typename R> NodeList* Generate(NodePtr<T, R>);
    template <std::string ComID, typename T, typename R> void Combine(R*, T*);
    template <std::string EvoID, typename T, typename R> void Evolve(R*, T*);
    template <std::string ComID, typename T, typename R> void TreeCombine(NodePtr<T,R>, NodePtr<T,R>);
    template class<typename T, typename R> BaseTree;

    //> Exception definition
    class TreeException : public std::runtime_error{
    public:
        TreeException(std::string const& message):std::runtime_error("Tree Error: "+message+" was Thrown.\n"){};

    };

    //> TreeNode definition
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


    template class<typename T, typename R> BaseTree {
    protected:
        NodePtr<T, R> Root; //< pointer to the root
        NodePtr<T, R> getCommonAncestor(NodePtr<T,R> u, NodePtr<T,R> v); //< get common ancestors of u and v
        void Insert(T* data_in); //< insert node with input value stored in data_in
        NodeList<T,R> TreeNodeList;  //< stores the entire list of the tree nodes. in case of array implementation, this should be a vector instead of list

    public: 
        BaseTree(std::ifstream); //< constructs a base tree from an input filestream 
        virtual ~BaseTree(); //< destructor
        void tree_compute(bool, std::string, std::string, std::string); //< calls generate or combine templates
    };

}

