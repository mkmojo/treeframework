//defines the tree interface
#include <string>
#include <set>
#include <list>
#include <exception>

namespace treedef {

    template<typename U, typename V> class TreeNode;
    typedef std::shared_ptr<TreeNode<U, V> > NodePtr; //U, V unknown types
    typedef std::list<TreeNode<U, V> > NodeList;
    template<std::string GenID, typename T, typename R> NodeList* Generate(NodePtr<T, R>);
    template <std::string ComID, typename T, typename R> void Combine(NodePtr<T,R>, NodePtr<T,R>);
    template class<typename T, typename R> BaseTree;

    class TreeException : public std::runtime_error{
    public:
        TreeException(std::string const& message):std::runtime_error("Tree Error: "+message+" was Thrown.\n"){};

    };

    template <typename U, typename V> class TreeNode {
        typedef std::shared_ptr<U> VarPtr;
        typedef std::shared_ptr<V> ComPtr;
        typedef std::set<NodePtr> GenSet;

    public:
        VarPtr pVar;
        ComPtr pCom;
        GenSet* pGen;
        NodePtr left, parent, sibling;

        TreeNode(U*);
        ~TreeNode();
        void getGenSet(NodeList*);
    };


    template class<typename T, typename R> BaseTree {
    protected:
        NodePtr Root;
        NodePtr getCommonAncestor(NodePtr u, NodePtr v);
        void Insert(T* data_in);
        NodeList TreeNodeList;

    public:
        BaseTree(std::ifstream);
        void tree_compute(bool, std::string, std::string);
        void tree_evolve(std::string);
    };

}

