#include<memory>
#include<vector>
namespace treedef {
    template < typename U, typename V> class TreeNode; 
    template < typename U, typename V> using NodePtr = std::shared_ptr<TreeNode<U, V> >;
    template < typename U, typename V> using NodeList = std::vector<std::shared_ptr<TreeNode<U,V> > >;

    template < typename U, typename V> class TreeNode {
        public:
        TreeNode();
        ~TreeNode();
    };
}
